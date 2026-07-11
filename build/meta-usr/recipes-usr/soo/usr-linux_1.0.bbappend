# Copyright (c) 2025-2026 EDGEMTech SA
# Adapted for MICOFE - Copyright (c) 2026 REDS Institute, HEIG-VD

SUMMARY = "Add-ons for SOO user space environment"
DESCRIPTION = "Additional applications are used to manage SO3 capsules"
LICENSE = "GPLv2"

# The SOO agency apps (src/soo, include/soo, include/core) are committed
# in-tree under linux/usr and built in place — no patch materialization.
# src/CMakeLists.txt permanently carries add_subdirectory(soo). Only the
# install step (staging the built binaries into the deploy dir) remains,
# gated on the :soo override.

# Installing usr apps mean to move the binary and all files which need to
# be copied to the rootfs. Be aware that it is a deploy directory and not
# the rootfs itself; this is achieved with the do_deploy task (by the bsp recipe)

do_install_apps:append () {

    if echo ":${OVERRIDES}:" | grep -q ":soo"; then
        usr_do_install_file_root "${IB_TARGET}/build/src/soo/injector"
        usr_do_install_file_root "${IB_TARGET}/build/src/soo/restoreme"
        usr_do_install_file_root "${IB_TARGET}/build/src/soo/saveme"
        usr_do_install_file_root "${IB_TARGET}/build/src/soo/melist"
        usr_do_install_file_root "${IB_TARGET}/build/src/soo/shutdownme"

        usr_do_install_file_root "${IB_TARGET}/build/src/soo/fb_mapper"
        usr_do_install_file_root "${IB_TARGET}/build/src/soo/input_forwarding"

        usr_do_install_file_root "${IB_TARGET}/build/src/soo/emiso_engine/emiso_engine"
    fi
}
