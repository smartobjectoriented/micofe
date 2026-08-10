# Copyright (c) 2025-2026 EDGEMTech SA
# Adapted for MICOFE - Copyright (c) 2026 REDS Institute, HEIG-VD

SUMMARY = "AVZ Hypervisor"
DESCRIPTION = "AVZ (Agency Virtualizer) hypervisor based on polymorphic SO3 Operating System"
LICENSE = "GPLv2"

inherit avz

# Version and revision

PR = "r0"
PV = "6.2.5"

OVERRIDES += ":avz"

# Where the working directory will be placed in infrabase root dir
IB_TARGET = "${IB_AVZ_PATH}"

# MICOFE is a separate repository and does not embed the SO3/AVZ sources, so
# AVZ is FETCHED from the SO3 repo (unlike the SO3 tree itself, which builds
# in place). Pinned to the SO3 release tag IB_SO3_TAG (see below;
# the bitbake git fetcher takes the tag's commit as
# SRCREV; IB_SO3_TAG records the human-readable tag it corresponds to.
IB_SO3_TAG = "v6.2.5"
SRC_URI = "git://github.com/smartobjectoriented/so3.git;nobranch=1;protocol=https"
SRCREV = "941c95c656d77bf5b56fdc3dc5d2df67d7923fcc"

# MICOFE-specific AVZ patches applied on top of the fetched SO3 tree.
# Generate/refresh the set with `bitbake avz -c updiff` (do_diffcompose
# diffs the fetched .pristine snapshot against ${IB_AVZ_PATH} and do_updiff
# regenerates the numbered patchset + this .inc).
FILESPATH:prepend = "${THISDIR}/files/0001-${PF}:"

require files/0001-${PF}-patches.inc

python do_handle_fetch_git() {

    import os
    import subprocess

    # Copy only the SO3 kernel (AVZ is the polymorphic SO3 built with an avz
    # config; its sources live inside the kernel tree). The repo nests the
    # kernel under <root>/so3/so3, so copy from gitdir/so3/so3.

    gitdir = os.path.join(d.getVar('WORKDIR'), 'git')
    dst_dir = d.getVar('S')

    cmd = f"find . -not -path '*/.git/*' -and \( -type f -or -type d -empty \) -exec cp -r --parents -t {dst_dir} {{}} +"
    result = subprocess.run(cmd, shell=True, check=True, cwd=os.path.join(gitdir, "so3", "so3"))
}

do_configure[nostamp] = "1"
do_configure () {
	cd ${IB_TARGET}
	make ${IB_CONFIG}
}

do_build[nostamp] = "1"
do_build () {
	echo "Building AVZ..."

	cd ${IB_TARGET}
	make
}

do_clean[nostamp] = "1"
do_clean () {
	rm -f ${TMPDIR}/stamps/avz*
}
addtask do_clean
