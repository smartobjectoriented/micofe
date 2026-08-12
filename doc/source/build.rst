.. _build:

Build system
############

MICOFE is built with the **Infrabase** build system (bitbake based), aligned on
the SO3 release the framework is pinned to (currently the ``v6.3.0`` tag).
The build tree follows the SO3 reference model: shared recipes and scripts are
kept identical to SO3, and only genuine MICOFE additions (the framebuffer/input
forwarding, the EMISO engine, the agency applications) diverge.

Source model
************

The components fall into two categories:

* **Fetched** components: ``so3`` (the capsule kernel), ``avz`` (the
  hypervisor), ``u-boot`` and ``qemu`` are fetched at build time from their
  upstream repositories. SO3 and AVZ are pinned to an SO3 release tag through
  ``SRCREV`` / ``IB_SO3_TAG`` in ``build/meta-so3/recipes-so3/{so3,avz}``. The
  fetched trees at the repository root (``so3/``, ``avz/``, ``u-boot/``,
  ``qemu/``) are build products and are not committed.

* **In-tree** components: the Linux agency kernel patches live in
  ``build/meta-linux`` — the SOO agency set follows the SO3 ``soo-generic``
  model (one generic patch directory shared by every agency kernel, plus a
  per-kernel directory whose same-named patches shadow the generic ones);
  the MICOFE vfbdev/vinput backends and their wiring are carried directly
  in MICOFE's copy of the generic set — and the MICOFE user space applications are committed
  under ``linux/usr`` (``src/soo`` holds the capsule management applications,
  the framebuffer/input forwarders and the EMISO engine). They are built in
  place — no patch materialization.

Building
********

The entry point is ``scripts/build.sh`` with a positional recipe name::

   ./scripts/build.sh bsp-linux       # full agency BSP (kernel, u-boot, rootfs, usr, ITBs)
   ./scripts/build.sh bsp-capsules    # the SO3 capsule image (virt64_capsule.itb)
   ./scripts/build.sh so3             # a single component (also: avz, uboot, usr-linux, ...)
   ./scripts/build.sh -l              # list the available recipes
   ./scripts/build.sh -c bsp-linux    # clean, then rebuild

For a fast edit/build loop on the user-space applications,
``scripts/makeusr.sh`` (run from ``linux/usr``) does a direct cmake+make —
including the out-of-tree kernel modules — without driving bitbake;
``deploy.sh usr-linux`` then pushes the result onto the media.

bitbake runs unprivileged; the few privileged operations (losetup, mkfs,
rootfs extraction) go through ``sudo -n``. Run
``scripts/common/setup_sudo.sh`` once so the sudo timestamp is shared with the
build (it installs a sudoers drop-in enabling global timestamps).

Building ``bsp-linux`` bakes the agency applications **into**
``rootfs.cpio``: the user space is compiled, then injected into the rootfs
archive as a build step. Deploying never rebuilds nor modifies that archive.

Containerized build (dbuild.sh)
===============================

The whole build environment (cross toolchains — including the bare-metal
``aarch64-none-elf`` AVZ/SO3 toolchain that is so easy to miss on a
hand-provisioned host — host packages, Python) is available as a Docker
image, defined under ``docker/build-env`` and driven by
``scripts/dbuild.sh``::

   ./scripts/dbuild.sh --build            # build the micofe-build:1.0 image (once)
   ./scripts/dbuild.sh build.sh bsp-linux # run any front-end script inside
   ./scripts/dbuild.sh                    # interactive shell, env.sh sourced
   ./scripts/dbuild.sh st.sh -d           # graphical QEMU from the container

The image carries only the environment: the repository stays on the host
and is bind-mounted **at its own absolute path**, so bitbake stamps, the
buildroot host tree and the CMake caches remain valid and a tree can be
built from inside or outside the container interchangeably. The container
runs as the calling host user (UID/GID mapped at run time) and carries a
blanket sudo rule, so the privileged storage steps never prompt.

Image assembly (ITS render)
===========================

The FIT image sources (``.its``) are **templates** committed in the layers
(``build/meta-bsp/recipes-bsp/{linux,so3}/files/its/``). At build time the
shared ``do_render_its`` task expands their ``${IB_*_PATH}`` placeholders and
renders them into the git-ignored ``linux/images/`` (or ``so3/images/``),
where ``do_itb`` runs ``mkimage``. The embedded ramfs is selected by
``IB_RAMFS_SOURCE`` in ``build/conf/local.conf``:

* ``"initrd"`` (MICOFE default) — the small, git-tracked
  ``board/<plat>/initrd.cpio`` busybox ramdisk. Its ``/init`` mounts the ext4
  rootfs on the second partition and ``switch_root``'s into it (two-stage
  boot).
* ``"rootfs"`` — the full buildroot ``rootfs.cpio``, run from RAM.

Deploying
*********

``scripts/deploy.sh`` is a pure media write (nothing is rebuilt)::

   ./scripts/deploy.sh bsp-linux      # ITBs + uEnv.txt on p1, agency rootfs on p2
   ./scripts/deploy.sh bsp-capsules   # capsule .itb into p2:/mnt/capsules/image/

The target is the ``filesystem/work/sdcard.img.<plat>`` image
(``IB_STORAGE_MODE = "soft"``), created once with
``./scripts/build.sh filesystem``.

Booting
*******

``scripts/st.sh`` boots the deployed image in QEMU (serial console on stdio).
The AVZ boot is a **two-ITB chain**: U-Boot loads ``virt64_avz.itb`` (the
hypervisor) and ``virt64_guest.itb`` (the Linux agency), then jumps to AVZ
through the ``guest-boot`` command (AVZ FIT in ``x0``, guest ITB in ``x1``).
``st.sh`` detects the AVZ configuration from the selected ITS and enables EL2
(``-M virt,virtualization=on``) automatically.

The agency then performs the two-stage boot: the embedded busybox ramdisk
mounts the full rootfs on ``/dev/vda2`` and switches into it, up to the MICOFE
login on the serial console.

Target platforms
****************

The platform is selected by ``IB_PLATFORM`` in ``build/conf/local.conf``.
When switching platforms, purge the per-recipe work directories and stamps
(``build/tmp/work``, ``build/tmp/stamps``) together with the in-place source
trees (``so3/``, ``avz/``, ``linux/usr/build``) — the fetched/attached trees
are replaced by the new platform's sources and a stale tree silently builds
the wrong target.

virt64 (QEMU)
=============

The development and validation platform. The whole stack is runtime-validated
there: agency boot, capsule injection (``s3c-*`` tools and the EMISO REST
API), the per-capsule VLOGS logs and the virtualized framebuffer chain. ``st.sh -d``
opens the QEMU GTK window showing the SO3/PL111 screen; the agency real
framebuffer additionally requires modern virtio::

   ./scripts/st.sh "-global virtio-mmio.force-legacy=false -device virtio-gpu-device"

rpi4_64 (Raspberry Pi 4)
========================

Set ``IB_PLATFORM ?= "rpi4_64"`` and build as usual — the full chain compiles
up to the two AVZ boot images (``rpi4_64_avz.itb`` and
``rpi4_64_linux_guest.itb`` under ``linux/images/``): the SO3 capsule, AVZ
(``rpi4_64_avz_soo_defconfig``), the Raspberry Pi kernel with the SOO agency
patch set (including the vfbdev/vinput backends), the ``bcm2711-rpi-4-b.dtb``
and the buildroot root file system. The AVZ guest ITS load addresses follow
the rpi4 memory map (kernel ``0x10000000``, FDT ``0x15000000``, initrd
``0x15c00000``; AVZ at ``0x00080000``).

.. note::

	The rpi4_64 environment is **build-validated only** — it has not been
	booted on a board yet. Flashing, the U-Boot/`guest-boot` bring-up and
	the framebuffer test on the real scanout are the next step.

.. note::

	The user-space test module for rpi4_64 is the generic ``modtry``; the
	Sense-HAT ``senseled`` module requires the ``VSENSELED``/``VSENSEJ``
	kernel backends, which have never been enabled in the rpi4 defconfig.

Capsule lifecycle
*****************

The capsule management applications shipped in the agency ``/root`` are the
SO3 ``s3c-*`` tools::

   s3c-inject /mnt/capsules/image/virt64_capsule.itb   # inject and start a capsule
   s3c-list                                            # list residing capsules and their state
   s3c-save <file>                                     # snapshot a capsule
   s3c-restore <file>                                  # restore a snapshot
   s3c-shutdown <id>                                   # shut a capsule down

They talk to the SOO core driver through ``/dev/soo/core`` (char device,
major 126), created at boot by the ``/etc/init.d/S30soo`` script (the agency
runs SysVinit). A living capsule exposes its console through the vuart
backend, multiplexed on the agency serial console.

The :ref:`EMISO engine <emiso>` offers the same lifecycle through a
Docker-compatible REST API.
