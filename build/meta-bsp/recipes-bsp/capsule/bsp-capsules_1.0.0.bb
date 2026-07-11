# Copyright (c) 2025-2026 EDGEMTech SA
# Adapted for MICOFE - Copyright (c) 2026 REDS Institute, HEIG-VD

SUMMARY = "SO3 capsule Deployment"
DESCRIPTION = "SO3 capsules are aimed to run with Linux as guest on top of the AVZ hypervisor."

LICENSE = "GPLv2"

# Version and revision
PV = "1.0.0"
PR = "r0"

inherit filesystem
inherit uboot
inherit logging
inherit bsp

# :append (not +=) so no space is inserted before ":so3" — otherwise the
# preceding CPU token parses as "arm "/"aarch64 " and :<cpu> overrides
# stop matching. See usr-so3_1.0.bb for the full rationale.
OVERRIDES:append = ":so3"

# bsp-capsules does not `inherit so3`, so IB_SO3_PATH (defined in so3.bbclass)
# is unset here — define it so the capsule ITS render resolves so3.bin/dtb.
IB_SO3_PATH ?= "${IB_DIR}/so3"

do_configure[noexec] = "1"
do_attach_infrabase[noexec] = "1"

# Building all components

do_build[depends] = "usr-so3:do_build" 

do_build () {
	bbplain "Everything built OK ..."
}
addtask do_build

####################### Recipe to deploy everything

def __do_platform_deploy(d):

    import os
    import subprocess

    # FOLLOW-UP (deferred): auto-place the capsule ITB into the booted image.
    # The ITB must land on the agency's storage p2 (/mnt/capsules/image), but a
    # standalone `deploy.sh bsp-capsules` does not mount the real sdcard here, so
    # this copy currently misses the booted image. For now place it by hand:
    #   loop-mount filesystem/work/sdcard.img.<plat> p2, cp so3/target/
    #   virt64_capsule.itb into mnt/capsules/image/, umount.
    # A proper fix must mount/populate the real sdcard p2 (a bitbake shared-
    # namespace / parse-cache issue blocked wiring it into this task).

    capsule_path = d.getVar('IB_FILESYSTEM_PATH') + "/p2/mnt/capsules/image"
    itb_path = d.getVar('IB_ITB_PATH') + "/" + d.getVar('IB_TARGET_ITS') + ".itb"

    if not os.path.isfile(itb_path):
        bb.fatal(itb_path + " is missing ...")

    subprocess.run(['mkdir', '-p', capsule_path])
    subprocess.run(['cp', itb_path, capsule_path])

# Capture the layer's ITS-template dir at PARSE time (${THISDIR} re-expands to
# the base recipe dir at task generation). Since SO3 v6.2.0 the capsule ITS is
# no longer shipped as a static file in the fetched so3/target tree — it lives
# here as a template and is rendered into ${IB_ITB_PATH} at build time.
IB_ITS_SRC := "${THISDIR}/files/its"

# Render an ITS template from IB_ITS_SRC into IB_ITB_PATH, expanding the
# ${IB_*_PATH} placeholders to absolute build paths. The sed patterns use char
# classes ([$][{]...[}]) so bitbake leaves them untouched and only the
# replacement side is expanded. Mirrors so3's bsp_render_its.

bsp_render_its() {
	mkdir -p "${IB_ITB_PATH}"
	sed -e "s|[$][{]IB_SO3_PATH[}]|${IB_SO3_PATH}|g" \
	    -e "s|[$][{]IB_ROOTFS_PATH[}]|${IB_ROOTFS_PATH}|g" \
	    -e "s|[$][{]IB_PLATFORM[}]|${IB_PLATFORM}|g" \
	    "${IB_ITS_SRC}/$1.its" > "${IB_ITB_PATH}/$1.its"
}

do_itb[nostamp] = "1"
do_itb[depends] = "usr-so3:do_deploy"
do_itb () {

	if [ ! -f ${IB_ITS_SRC}/${IB_TARGET_ITS}.its ]; then
		bbfatal "No ITS template found at ${IB_ITS_SRC}/${IB_TARGET_ITS}.its"
	fi

	bsp_render_its ${IB_TARGET_ITS}
	mkimage -f ${IB_ITB_PATH}/${IB_TARGET_ITS}.its ${IB_ITB_PATH}/${IB_TARGET_ITS}.itb
}

do_deploy[depends] = "usr-so3:do_deploy"
 
do_deploy[nostamp] = "1"
python do_deploy() {
    
    bb.plain("Deploy SO3 image and U-boot")

    __do_deploy_boot(d);
}

addtask do_itb before do_deploy
addtask do_deploy

do_deploy_boot[nostamp] = "1"
python do_deploy_boot() {

    bb.plain("Deploy SO3 boot (u-boot, itb)")

    __do_deploy_boot(d)
}
addtask do_itb before do_deploy_boot
addtask do_deploy_boot

do_clean[depends] = "usr-so3:do_clean so3:do_clean uboot:do_clean"
do_clean[nostamp] = "1"
do_clean () {
	rm -f ${TMPDIR}/stamps/bsp-so3*
}
addtask do_clean

 
 

