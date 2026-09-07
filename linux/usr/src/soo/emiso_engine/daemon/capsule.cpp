/*
 * Copyright (C) 2023 Jean-Pierre Miceli <jean-pierre.miceli@heig-vd.ch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <chrono>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <iostream>

#include <string.h>
#include <unistd.h>

#include <zip.h>

#include <sys/ioctl.h>

#include <soo/uapi/soo.h>

#include "capsule.hpp"

using namespace std;

// Path where the Capsule images are stored
#define EMISO_IMAGE_PATH "/mnt/capsules/image/"

// Path where the Capsule snapshot are stored
#define EMISO_CAPSULE_CACHE_DIR "/mnt/capsules/snapshot"

// Path of SOO driver
#define SOO_CORE_DRV_PATH "/dev/soo/core"

#define LOG_PREFIX "[EMISO:DAEMON] "

namespace emiso {

map<int, CapsuleInfo> Capsule::_capsules;
unsigned Capsule::_capsuleIdx = 0;

Capsule::Capsule(){};

Capsule::~Capsule(){};

/**
 * @brief Return NOW in epoch format, in seconds
 *
 * It is used to set Capsule's creation time
 *
 * @return now, since EPOCH, in second
 */
uint64_t Capsule::createdTime()
{
    const auto now = chrono::system_clock::now();
    const auto epoch = chrono::duration_cast<chrono::seconds>(now.time_since_epoch()).count();

    return epoch;
}


/*
 * @brief  Inject a capsule
 *
 * @param imageName   Name image, without .itb suffix, used to inject the capsule
 * @return The ID of the create capsule or a negative value in case of error
 */
int Capsule::inject(string imageName, bool start_capsule, unsigned capsuleId)
{
    int fd;
    int ret;
    streampos capsuleSize;
    string filename;
    char *capsuleBuf;
    struct agency_ioctl_args args;

    cout << LOG_PREFIX "Capsule injection from image " << imageName << endl;

    filename = std::format("{}{}.itb", EMISO_IMAGE_PATH, imageName);

    ifstream image(filename, ios::in | ios::binary | ios::ate);
    if (!image.is_open()) {
        cerr << LOG_PREFIX "Error: Failed to open image file '" << filename << "'" << endl;

        return -1;
    }

    capsuleSize = image.tellg();

    capsuleBuf = new char[capsuleSize];

    image.seekg(0, ios::beg);
    image.read(capsuleBuf, capsuleSize);
    image.close();

    args.buffer = capsuleBuf;
    args.value = capsuleSize;
    args.slotID = -1; /* Wherever */
    args.capsuleID = capsuleId;

    fd = open(SOO_CORE_DRV_PATH, O_RDWR);
    if (fd < 0) {
        cerr << "[EMISO:DAEMON] Failing to open /dev soo entry ..." << endl;
        delete[] capsuleBuf;

        return -1;
    }

    /* Inject the capsule */
    ret = ioctl(fd, AGENCY_IOCTL_INJECT_CAPSULE, &args);
    if ((ret < 0) || (args.slotID == -1)) {
        cerr << "[EMISO:DAEMON] No available ME slot further..." << endl;
        close(fd);
        delete[] capsuleBuf;

        return -1;
    }

    if (start_capsule) {
        ioctl(fd, AGENCY_IOCTL_START_CAPSULE, &args);
    }

    delete[] capsuleBuf;
    close(fd);

    return args.slotID;
}


/*
 * @brief  Save a snapshot of a Capsule
 *
 * @param slotId The slot ID of the capsule for which to create a snapshot.
 * @param snapshotName The name of the snapshot
 * @return negative value in case of error, 0 otherwise
 */
int Capsule::saveSnapshot(int slotId, string snapshotName)
{
    int fd;
    int ret;
    struct agency_ioctl_args args;
    struct zip_t *zip;
    string filename;

    cout << LOG_PREFIX "Save Capsule - slot ID: " << slotId << ", name: " << snapshotName << endl;

    fd = open(SOO_CORE_DRV_PATH, O_RDWR);
    if (fd < 0) {
        printf("[EMISO:DAEMON] Failing to open /dev/soo entry ...\n");

        return -1;
    }

    /* Get the size of the snapshot */
    args.slotID = slotId;
    args.value = 0; /* To get the size of the snapshot */

    /* The HOLD flavour leaves the capsule suspended instead of resuming it: a
     * snapshot is only ever saved to pause the capsule, which is shut down
     * right after. Resuming it in between would let it run, and diverge from
     * the snapshot just taken, for nothing.
     *
     * What such a capsule cannot release itself is released for it: the agency
     * tears down its backends in shutdown_capsule(), and AVZ frees the grants
     * it still holds when the domain is destroyed.
     */

    ret = ioctl(fd, AGENCY_IOCTL_READ_SNAPSHOT_HOLD, &args);
    if ((ret < 0) || (args.value == 0)) {
        printf(LOG_PREFIX "Get the size with IOCTL_READ_SNAPSHOT failed.\n");
        close(fd);

        return -1;
    }

    args.buffer = new (std::nothrow) char[args.value];
    if (args.buffer == NULL) {
        printf(LOG_PREFIX " %s - malloc failed\n", __func__);
        close(fd);

        return -1;
    }

    /* A snapshot which could not be read entirely must not be stored: the
     * kernel reports that with a failing ioctl.
     */

    ret = ioctl(fd, AGENCY_IOCTL_READ_SNAPSHOT_HOLD, &args);
    if (ret < 0) {
        printf(LOG_PREFIX "Read the snapshot IOCTL_READ_SNAPSHOT failed.\n");
        delete[] (char *) args.buffer;
        close(fd);

        return -1;
    }

    close(fd);

    filename = string(EMISO_CAPSULE_CACHE_DIR) + "/" + snapshotName;

    cout << "Save capsule in: " << filename << endl;

    /* Compress the snapshot */
    zip = zip_open(filename.c_str(), ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
    if (!zip) {
        printf(LOG_PREFIX "Failed to open the zip file. Is there a bad sync after saving the snapshot?...\n");
        perror("");
        delete[] args.buffer;
        close(fd);
        return EXIT_FAILURE;
    }
    zip_entry_open(zip, "me");
    zip_entry_write(zip, args.buffer, args.value);
    zip_entry_close(zip);

    zip_close(zip);
    delete[] args.buffer;

    return 0;
}

/*
 * @brief  Restore a Capsule for a snapshot
 *
 * @param snapshotName The name of the snapshot
 * @return The slot ID used for the capsule or a negative value in case of error
 */
int Capsule::restoreSnapshot(string snapshotName)
{
    int fd;
    int ret;
    struct agency_ioctl_args args;
    size_t buffer_size;
    struct zip_t *zip;
    string filename;

    /* Open the channel to the kernel soo submodule. */
    fd = open(SOO_CORE_DRV_PATH, O_RDWR);
    if ((fd < 0)) {
        printf(LOG_PREFIX "Failed to open soo /dev entry...\n");

        return -1;
    }

    filename = std::string(EMISO_CAPSULE_CACHE_DIR) + "/" + snapshotName;

    zip = zip_open(filename.c_str(), 0, 'r');
    if (!zip) {
        printf(LOG_PREFIX "Failed to open the zip file. Is there a bad sync after saving the snapshot?...\n");
        close(fd);

        return -1;
    }

    /* The buffer is allocated by zip_entry_read(). It has to be initialized
     * beforehand: should the entry be missing or unreadable, an uninitialized
     * pointer would end up in the ioctl and the kernel would read a bogus
     * snapshot size out of it.
     */

    args.buffer = nullptr;
    buffer_size = 0;

    if (zip_entry_open(zip, "me") < 0) {
        printf(LOG_PREFIX "No 'me' entry in snapshot '%s'...\n", snapshotName.c_str());
        zip_close(zip);
        close(fd);

        return -1;
    }

    if ((zip_entry_read(zip, &args.buffer, &buffer_size) < 0) || (args.buffer == nullptr)) {
        printf(LOG_PREFIX "Failed to read the 'me' entry of snapshot '%s'...\n", snapshotName.c_str());
        zip_entry_close(zip);
        zip_close(zip);
        close(fd);

        return -1;
    }

    zip_entry_close(zip);

    zip_close(zip);

    /* Restore the snapshot which is in <stopped> stated */
    cout << LOG_PREFIX "Re-implementing snapshot of size " << buffer_size << " bytes." << endl;
    args.slotID = -1;
    ret = ioctl(fd, AGENCY_IOCTL_WRITE_SNAPSHOT, &args);

    free(args.buffer);

    if (ret < 0) {
        printf("Failed to initialize migration (%d)\n", ret);
        close(fd);

        return -1;
    }

    close(fd);

    return args.slotID;
}

/*
 * @brief  Shutdown a Capsule
 *
 * @param slotId The slot ID of the capsule to shutdown.
 * @return a negative value in case of error, 0 otherwise
 */
int Capsule::shutdown(int slotId)
{
    int fd;
    int ret;
    struct agency_ioctl_args args;

    fd = open(SOO_CORE_DRV_PATH, O_RDWR);
    if (fd < 0) {
        printf("[EMISO:DAEMON] Failing to open /dev/soo entry ...\n");

        return -1;
    }

    /* Shutdown the capsule so that it will be removed from the memory */
    args.slotID = slotId;

    ret = ioctl(fd, AGENCY_IOCTL_SHUTDOWN, &args);
    if (ret < 0) {
        printf(LOG_PREFIX "IOCTL_SHUTDOWN failed for slot %d.\n", slotId);
        close(fd);

        return -1;
    }

    close(fd);

    return 0;
}

/**
 * @brief Retrieve the info of all the capsules
 *
 * @param capsuleList A map which contains the info of all capsules.
 */
void Capsule::info(map<int, CapsuleInfo> &capsuleList)
{
    capsuleList = _capsules;
}


/**
 * @brief Retrieve the info of a specific capsule
 *
 * @param id  ID of the capsule from which to retrieve the information
 * @param info The info of the capsule
 */
void Capsule::info(int id, CapsuleInfo &info)
{
    info = _capsules.at(id);
}


/**
 * @brief  Creating a capsule only registers it: nothing is injected yet.
 *
 *         A capsule which has never been scheduled has no CPU context to take a
 *         snapshot of: AVZ fills the vcpu of a domain (VBAR_EL1, SCTLR_EL1, the
 *         translation table registers) when it schedules it out, so the snapshot
 *         of a freshly injected capsule carries zeros there. Restoring it gives
 *         a domain with no vector table and no MMU setup, which faults on its
 *         first exception. Hence a created capsule is injected when it is
 *         started, exactly like an exited one.
 *
 * @param imageName   Name image, without .itb suffix, used to create the capsule
 * @param capsuleName The name of the capsule to create
 * @return The ID of the created capsule, or a negative value in case of error
 */
int Capsule::create(string imageName, string capsuleName)
{
    string filename;

    cout << LOG_PREFIX "Create capsule " << capsuleName << " from image " << imageName << endl;

    /* The image is not read here, but no capsule may be registered for an image
     * which does not exist.
     */

    filename = std::format("{}{}.itb", EMISO_IMAGE_PATH, imageName);

    ifstream image(filename, ios::in | ios::binary);
    if (!image.is_open()) {
        cerr << LOG_PREFIX "Error: Failed to open image file '" << filename << "'" << endl;

        return -1;
    }

    image.close();

    // Save the info of the new capsule
    CapsuleInfo capsule;
    capsule.id      = _capsuleIdx;
    capsule.name    = capsuleName;
    capsule.state   = "created";
    capsule.image   = imageName;
    capsule.slotId  = -1; /* No slot is held until the capsule is started */
    capsule.created = this->createdTime();

    _capsules[_capsuleIdx] = capsule;
    _capsuleIdx++;

    return capsule.id;
}

/**
 * @brief Start an existing capsule
 *
 *  A capsule which has never run -- created or exited -- is (re)injected from
 *  its image. Resuming a paused capsule goes through unpause(), which restores
 *  its snapshot.
 *
 * @param capsuleId
 * @return The slot ID the capsule runs in, or a negative value in case of error
 */
int Capsule::start(unsigned capsuleId)
{
    string capsuleName = _capsules[capsuleId].name;
    int slotId;

    cout << LOG_PREFIX "Start capsule " << capsuleName << endl;

    if ((_capsules[capsuleId].state == "created") || (_capsules[capsuleId].state == "exited")) {
        slotId = this->inject(_capsules[capsuleId].image, true, capsuleId);

    } else {
        cerr << "[EMISO:DAEMON] Capsule start failed. The capsule it is not in a correct state ("
             << _capsules[capsuleId].state << ")" << endl;
        return -1;
    }

    if (slotId < 0) {
        cerr << LOG_PREFIX "Capsule start failed for " << capsuleName << endl;

        return -1;
    }

    _capsules[capsuleId].state  = "running";
    _capsules[capsuleId].slotId = slotId;

    return slotId;
}


/**
 * @brief Shutdown a capsule
 *
 * @param capsuleId ID of the capsule to shutdown
 * @return 0 in case of success or a negative value
 */
int Capsule::stop(unsigned capsuleId)
{
    int ret;
    struct agency_ioctl_args args;

    ret = this->shutdown(_capsules[capsuleId].slotId);

    _capsules[capsuleId].state  = "exited";

    return ret;
}


/**
 *  @brief Restart a capsule
 *
 *  @param capsuleId ID of the capsule to shutdown
 *  @return 0 in case of success or a negative value
 */
int Capsule::restart(unsigned capsuleId)
{
    int ret;

    auto imageName   = _capsules[capsuleId].image;
    auto capsuleName = _capsules[capsuleId].name;

    // Stop & remove the capsule
    ret = this->shutdown(capsuleId);
    if (ret < 0)
        return ret;

    // re-create the capsule
    ret = this->inject(imageName,  true, capsuleId);
    if (ret < 0)
        return ret;

    _capsules[capsuleId].state  = "running";

    return 0;
}


/**
 * @brief Pause a capsule
 *
 * @param capsuleId ID of the capsule to pause
 * @return 0 in case of success or a negative value
 */
int Capsule::pause(unsigned capsuleId)
{
    int ret;

    cout << LOG_PREFIX "Pause capsule with ID " << capsuleId << endl;

    /* The capsule keeps running if its snapshot could not be taken: shutting it
     * down here would lose it for good.
     */

    ret = this->saveSnapshot(_capsules[capsuleId].slotId, _capsules[capsuleId].name);
    if (ret < 0) {
        cerr << LOG_PREFIX "Pause failed: no snapshot taken of capsule " << capsuleId << endl;

        return -1;
    }

    ret = this->shutdown(_capsules[capsuleId].slotId);
    if (ret < 0)
        return ret;

    _capsules[capsuleId].state = "paused";

    return 0;
}


/**
 * @brief Unpause a capsule
 *
 * @param capsuleId ID of the capsule to pause
 * @return 0 in case of success or a negative value
 */
int Capsule::unpause(unsigned capsuleId)
{
    int slotId;

    slotId = this->restoreSnapshot(_capsules[capsuleId].name);
    if (slotId < 0) {
        cerr << LOG_PREFIX "Unpause failed for capsule " << capsuleId << endl;

        return -1;
    }

    /* The snapshot is re-implanted wherever a slot was free. */

    _capsules[capsuleId].slotId = slotId;
    _capsules[capsuleId].state  = "running";

    return 0;
}

/**
 * @brief Remove a capsule
 *
 * @param capsuleId ID of the capsule to pause
 * @return 0 in case of success or a negative value
 */
int Capsule::remove(unsigned capsuleId)
{
    int ret = 0;

    if (_capsules[capsuleId].state == "running")
        ret = this->shutdown(_capsules[capsuleId].slotId);

    if (ret == 0)
        _capsules.erase(capsuleId);

    return ret;
}

vector<string> Capsule::retrieveLogs(unsigned capsuleId, unsigned lineNr)
{
    vector<string> lines;

    // Create the file path
    string fileName = "/var/log/soo/s3c_" + to_string(capsuleId) + ".log";

    cout << "[DEBUG] Logfile name: " << fileName << endl;

    // Read the file
    ifstream file(fileName);

    if (!file.is_open()) {
        cerr << "Error opening file: " << fileName << endl;
        return lines; // return empty vector if file couldn't be opened
    }

    // Read the whole file
    if (lineNr == -1) {
        // Count the number of lines
        lineNr = count(istreambuf_iterator<char>(file),
                       istreambuf_iterator<char>(), '\n');


        // Reset the file stream back to beginning
        file.clear();
        file.seekg(0, ios::beg);
    }

    string line;
    while (getline(file, line) && lineNr-- > 0) {
        lines.push_back(line);
    }

    file.close();
    return lines;
}

} // namespace emiso
