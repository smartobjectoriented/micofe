#!/bin/bash

# Copyright (c) 2025-2026 EDGEMTech SA
# Adapted for MICOFE - Copyright (c) 2026 REDS Institute, HEIG-VD

# Resolve project root from this script's own location, cd there, and
# source env.sh — prompting the user first if the parent shell points
# at a different tree. Every relative path below (filesystem/...,
# build/conf/local.conf) is anchored on that root. See
# scripts/common/setup_env.sh.

. "$(cd "$(dirname "$(command -v -- "$0")")" && pwd)/common/setup_env.sh"

QEMU_AUDIO_DRV="none"
GDB_PORT_BASE=1234
SSH_PORT_BASE=2222
EMISO_PORT_BASE=2375

# Parse our own options (currently just -d) out of the argument list before
# what's left is forwarded to QEMU as USR_OPTION.
WITH_DISPLAY=0
POSARGS=()
for _a in "$@"; do
    case "$_a" in
        -d) WITH_DISPLAY=1 ;;
        -h|--help)
            echo "Usage: $(basename "$0") [-d] [qemu-option]"
            echo "  -d   graphical: open the QEMU GTK window showing the guest PL111/LVGL screen"
            echo "       (default: headless, serial console only)"
            exit 0 ;;
        *)  POSARGS+=("$_a") ;;
    esac
done
set -- "${POSARGS[@]}"
USR_OPTION=$1
# QEMU_BIN is selected per IB_PLATFORM below (qemu-system-aarch64 for
# virt64, qemu-system-arm for virt32).

# One guest at a time. Every instance attaches the same
# filesystem/sdcard.img.<platform> with file.locking=off, so a second one
# writes into the ext4 the first is already writing to -- silently, and the
# snapshots a capsule saves live on that very partition. Refuse to start
# rather than let two guests corrupt the image.

RUNNING_QEMU=$(pgrep -f 'qemu-system-[a-z0-9]+ ' | tr '\n' ' ')
if [ -n "${RUNNING_QEMU}" ]; then
    printf "Error: a QEMU guest is already running (pid %s).\n" "${RUNNING_QEMU% }" >&2
    printf "       Quit it first: Ctrl-A x in its console, or kill %s\n" "${RUNNING_QEMU% }" >&2
    exit 1
fi

launch_qemu() {
    QEMU_MAC_ADDR="DE:AD:BE:EF:00:00"

    GDB_PORT=${GDB_PORT_BASE}

    # Slirp host->guest port forwards.
    #   :22   -> guest SSH. No sshd in the current buildroot agency rootfs
    #            (BR2_PACKAGE_DROPBEAR unset), kept for rootfs variants that
    #            ship one.
    #   :2375 -> EMISO engine, the Docker-subset REST API served by
    #            /root/emiso_engine. This is what a Portainer Server running on
    #            the host PC talks to when the agency is registered as a
    #            "Docker Standalone / API" environment — doc/source/portainer.rst.
    # Both bind 0.0.0.0 on the host, so they are reachable from the LAN and from
    # a containerised Portainer (via the docker0 gateway). Extra forwards can be
    # added without editing this file:
    #   IB_QEMU_HOSTFWD="tcp::9000-:9000,tcp::1880-:1880" st.sh

    SSH_PORT=${SSH_PORT_BASE}
    EMISO_PORT=${EMISO_PORT_BASE}
    HOSTFWD_OPT="hostfwd=tcp::${SSH_PORT}-:22,hostfwd=tcp::${EMISO_PORT}-:2375"
    if [ -n "${IB_QEMU_HOSTFWD}" ]; then
        HOSTFWD_OPT="${HOSTFWD_OPT},hostfwd=${IB_QEMU_HOSTFWD//,/,hostfwd=}"
    fi
    NETDEV_OPT="user,id=n1,${HOSTFWD_OPT}"

    echo -e "\033[01;36mMAC addr: " ${QEMU_MAC_ADDR} "\033[0;37m"
    echo -e "\033[01;36mGDB port: " ${GDB_PORT} "\033[0;37m"
    echo -e "\033[01;36mSSH port: " ${SSH_PORT} "-> guest :22\033[0;37m"
    echo -e "\033[01;36mEMISO port: " ${EMISO_PORT} "-> guest :2375 (Portainer endpoint)\033[0;37m"

    while IFS= read -r line; do
      # Check if the line starts with "IB_PLATFORM"
      if [[ $line == IB_PLATFORM* ]]; then
    	  # Extract the value between the quotes
    	  value=$(echo "$line" | awk -F'"' '{print $2}')
    
    	  # Set the IB_PLATFORM variable to the extracted value
    	  IB_PLATFORM="$value"
    	  break
      fi
    done < build/conf/local.conf

    # Detect an AVZ boot from the selected (uncommented) ITS. Both the so3
    # and the linux ITS must be checked: with the Linux agency + SO3 capsules
    # the so3 ITS is <plat>_capsule while the boot image is the linux one
    # (IB_TARGET_ITS:linux = <plat>_avz). AVZ is an EL2 hypervisor, so QEMU
    # must expose EL2 (virtualization=on); a standalone SO3 boots at EL1.
    SO3_ITS=$(grep -E "^IB_TARGET_ITS:so3:${IB_PLATFORM}\b" build/conf/local.conf | awk -F'"' '{print $2}' | tail -1)
    LINUX_ITS=$(grep -E "^IB_TARGET_ITS:linux:${IB_PLATFORM}\b" build/conf/local.conf | awk -F'"' '{print $2}' | tail -1)

    # Display mode. Default: headless (serial console only, -display none). With
    # -d: open the QEMU GTK window that presents the guest PL111 CLCD (the LVGL
    # screen). SO3 drives PL111 + PL050 (wired unconditionally into '-M virt' by
    # the so3 QEMU patch) and has no virtio-gpu, so no extra device flags are
    # needed — just switch the display backend. Use GTK, not SDL: SDL leaves the
    # PL111 console black, GTK presents it (and its View menu lists every
    # console). On a fractionally-scaled HiDPI Wayland panel, route GTK through
    # XWayland (GDK_BACKEND=x11) so the so3,absmouse absolute pointer maps 1:1
    # onto the guest surface; harmless on a real X11 session.
    if [ "$WITH_DISPLAY" == "1" ]; then
        DISPLAY_OPT="-display gtk,zoom-to-fit=off"
        export GDK_BACKEND=x11
        export GDK_SCALE=1
        export GDK_DPI_SCALE=1
    else
        DISPLAY_OPT="-display none"
    fi

    if [ "$IB_PLATFORM" == "virt64" ]; then
    QEMU_BIN="$IB_ROOT_DIR/qemu/build/qemu-system-aarch64"
    echo Starting on virt64
    # User-mode (slirp) networking: QEMU plays DHCP + DNS + NAT internally, so
    # the guest gets 10.0.2.15 immediately and NetworkManager-wait-online
    # succeeds in <1 s instead of timing out at 60 s as it did with tap+host
    # bridge that had no DHCP server. hostfwd exposes guest SSH on host
    # port 2222 and the EMISO engine on host port 2375 (see HOSTFWD_OPT).
    # Trade-off: the guest is NAT'd, it has no LAN address of its own — anything
    # to be reached from outside must be forwarded explicitly.
    # Bonus: no sudo needed (no tap device creation), so QEMU artefacts stay
    # owned by the regular user across runs.
    #
    # Boot mode is picked from artefacts in filesystem/ (built by bsp.bbclass
    # :do_deploy_boot_chain → bsp_virt64.inc:__do_platform_boot_chain):
    #   * flash0.img present → ATF chain (AVZ boot chain).
    #     QEMU exposes EL3 (secure=on) and pflash-loads BL1+FIP; EL2 enabled
    #     so AVZ (in "full" mode) or U-Boot's hyp-mode can run.
    #   * flash0.img absent → bare bsp-linux. The boot chain is just
    #     U-Boot + Linux; QEMU `-kernel`-loads the U-Boot ELF
    #     (u-boot/u-boot) at EL1 directly. EL2 (and EL3) are deliberately
    #     disabled — the qemu-arm64 U-Boot config expects to run at EL1,
    #     and running at EL2 without firmware handling PSCI / breaking
    #     the bootm path triggers a synchronous external abort during
    #     AMBA PL011 probe.

    if [ -f filesystem/flash0.img ]; then
        # ATF/OP-TEE chain (flash0/FIP): EL3 (secure=on) + EL2 enabled.
        MACHINE_OPT="-M virt,virtualization=on,gic-version=2,secure=on"
        BOOT_OPT="-drive if=pflash,format=raw,file=filesystem/flash0.img"
    elif [[ "$SO3_ITS" == *avz* || "$LINUX_ITS" == *avz* ]]; then
        # AVZ via the ITS, no ATF: AVZ is an EL2 hypervisor, so QEMU
        # must expose EL2 (virtualization=on). The U-Boot here is
        # virt64_defconfig (FIP/EL2-aware), so it runs fine at EL2 and hands
        # off to AVZ at EL2. Without this, AVZ faults on its first EL2
        # system-register access (Synchronous Abort -> reset).
        echo "AVZ guest (so3 ITS=$SO3_ITS, linux ITS=$LINUX_ITS) — enabling EL2 (virtualization=on)"
        MACHINE_OPT="-M virt,gic-version=2,virtualization=on"
        BOOT_OPT="-kernel u-boot/u-boot"
    else
        MACHINE_OPT="-M virt,gic-version=2"
        BOOT_OPT="-kernel u-boot/u-boot"
    fi
    ${QEMU_BIN} $@ ${USR_OPTION} \
		-smp 4  \
		-chardev stdio,id=char0,mux=on,signal=off \
		-mon chardev=char0 \
		-serial chardev:char0 \
		${MACHINE_OPT} -cpu cortex-a72  \
		${BOOT_OPT} \
		-device virtio-blk-device,drive=hd0 \
		-drive if=none,file=filesystem/sdcard.img.virt64,id=hd0,format=raw,file.locking=off \
		-m 1024 \
		${DISPLAY_OPT} \
		-netdev ${NETDEV_OPT} \
		-device virtio-net-device,netdev=n1,mac=${QEMU_MAC_ADDR} \
        	-gdb tcp::${GDB_PORT}
	fi

    if [ "$IB_PLATFORM" == "virt32" ]; then
    QEMU_BIN="$IB_ROOT_DIR/qemu/build/qemu-system-arm"
    echo Starting on virt32
    # SO3 standalone on 32-bit ARM virt: U-Boot is loaded directly with
    # -kernel (no ATF / flash chain on this platform), cortex-a15 matches
    # the SO3 virt32 build. Serial console is muxed onto stdio like virt64;
    # slirp user networking keeps QEMU artefacts owned by the regular user.
    ${QEMU_BIN} $@ ${USR_OPTION} \
		-smp 4  \
		-chardev stdio,id=char0,mux=on,signal=off \
		-mon chardev=char0 \
		-serial chardev:char0 \
		-M virt -cpu cortex-a15 \
		-kernel u-boot/u-boot \
		-device virtio-blk-device,drive=hd0 \
		-drive if=none,file=filesystem/sdcard.img.virt32,id=hd0,format=raw,file.locking=off \
		-m 1024 \
		${DISPLAY_OPT} \
		-netdev ${NETDEV_OPT} \
		-device virtio-net-device,netdev=n1,mac=${QEMU_MAC_ADDR} \
        	-gdb tcp::${GDB_PORT}
	fi


    QEMU_RESULT=$?
}

launch_qemu
