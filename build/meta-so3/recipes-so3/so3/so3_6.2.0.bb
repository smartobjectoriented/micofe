# Copyright (c) 2025-2026 EDGEMTech SA
# Adapted for MICOFE - Copyright (c) 2026 REDS Institute, HEIG-VD

SUMMARY = "SO3 kernel"
DESCRIPTION = "Smart Object Oriented Operating System"
LICENSE = "GPLv2"

inherit so3

# Version and revision
PR = "r0"
PV = "6.2.0"

# :append (not +=) so no space is inserted before ":so3" — otherwise the
# preceding CPU token parses as "arm "/"aarch64 " and :<cpu> overrides
# stop matching. See usr-so3_1.0.bb for the full rationale.
OVERRIDES:append = ":so3"

# Where the working directory will be placed in infrabase root dir
IB_TARGET = "${IB_SO3_PATH}"

# MICOFE is a separate repository and does not embed the SO3 sources, so the
# kernel is FETCHED from the SO3 repo (unlike the SO3 tree itself, which builds
# in place). Track the latest SO3 (branch 255: build-system→infrabase + SOO
# capsule + new logo). Bump SRCREV to follow upstream SO3 changes.
SRC_URI = "git://github.com/smartobjectoriented/so3.git;nobranch=1;protocol=https"
SRCREV = "60f6da75c8969d2710c0e98572f2a43febb6a34b"

# MICOFE-specific SO3 patches applied on top of the fetched tree.
# Generate/refresh the set with `bitbake so3 -c updiff` (do_diffcompose
# diffs the fetched .pristine snapshot against ${IB_SO3_PATH} and do_updiff
# regenerates the numbered patchset + this .inc).
FILESPATH:prepend = "${THISDIR}/files/0001-${PF}:"

require files/0001-${PF}-patches.inc

python do_handle_fetch_git() {

    import os
    import subprocess

    # Copy the SO3 sources subtree only. The repo nests its sources under
    # <root>/so3 (which itself contains so3/=kernel, usr/, rootfs/, target/),
    # so copy from gitdir/so3 — this makes ${IB_SO3_PATH}/so3 the kernel root,
    # matching the in-place SO3 build layout.

    gitdir = os.path.join(d.getVar('WORKDIR'), 'git')
    dst_dir = d.getVar('S')

    cmd = f"find . -not -path '*/.git/*' -and \( -type f -or -type d -empty \) -exec cp -r --parents -t {dst_dir} {{}} +"
    result = subprocess.run(cmd, shell=True, check=True, cwd=os.path.join(gitdir, "so3"))
}

do_configure[nostamp] = "1"
do_configure () {
	cd ${IB_SO3_PATH}/so3

	# The kernel is built in place, so a stale .config/objects from the
	# other architecture (virt64<->virt32, i.e. aarch64<->arm) would
	# otherwise survive and produce a wrong-arch kernel. Track the last
	# built arch in a marker file and distclean only when it changes —
	# so same-arch rebuilds stay incremental.
	_arch_marker=".ib_last_arch"
	if [ -f "$_arch_marker" ] && [ "$(cat $_arch_marker)" != "${IB_PLAT_CPU}" ]; then
		echo "SO3 arch changed ($(cat $_arch_marker) -> ${IB_PLAT_CPU}); running make distclean"
		make distclean
	fi

	make ${IB_CONFIG}

	echo "${IB_PLAT_CPU}" > "$_arch_marker"
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
