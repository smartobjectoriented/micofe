.. _build:

Build system
############

MICOFE is built with the **Infrabase** build system (bitbake based), aligned on
the SO3 release the framework is pinned to (currently the ``v6.2.1-rc`` tag).
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
  ``build/meta-linux``, and the MICOFE user space applications are committed
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

bitbake runs unprivileged; the few privileged operations (losetup, mkfs,
rootfs extraction) go through ``sudo -n``. Run
``scripts/common/setup_sudo.sh`` once so the sudo timestamp is shared with the
build (it installs a sudoers drop-in enabling global timestamps).

Building ``bsp-linux`` bakes the agency applications **into**
``rootfs.cpio``: the user space is compiled, then injected into the rootfs
archive as a build step. Deploying never rebuilds nor modifies that archive.

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
