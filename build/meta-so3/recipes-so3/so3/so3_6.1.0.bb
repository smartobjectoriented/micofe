
SUMMARY = "SO3 kernel"
DESCRIPTION = "Smart Object Oriented Operating System"
LICENSE = "GPLv2"

inherit so3

# Version and revision
PR = "r0"
PV = "6.1.0"

OVERRIDES += ":so3"

# Where the working directory will be placed in infrabase root dir
IB_TARGET = "${IB_SO3_PATH}"

SRC_URI = "git://github.com/smartobjectoriented/so3.git;nobranch=1;protocol=https"
SRCREV = "5826b0c0266cd561b82925eea4ad8fdc8bceef79"

# FILESPATH:prepend = "${THISDIR}/files/0001-${PF}:"
# require files/0001-${PF}-patches.inc

do_configure[nostamp] = "1"
do_configure () {
	cd ${IB_SO3_PATH}/so3
	make ${IB_CONFIG}
}

do_build () {
	echo "Building SO3..."
	
	cd ${IB_SO3_PATH}/so3
	make
}

do_clean[nostamp] = "1"
do_clean () {
	rm -f ${TMPDIR}/stamps/so3*
}
addtask do_clean
