
Glossary
########

.. glossary::
   :sorted:

   MICOFE
      Micro-Container for Edge Computing. The project that provides a lightweight,
      strongly isolated micro-container environment for edge computing, built on
      :term:`SO3` and Arm64 virtualization.

   SO3
      A lightweight operating system supporting key Linux-like features such as
      user/kernel separation, memory paging, and multithreading. It is the
      operating system running inside the :term:`capsule`.

   SOO
      The virtualization framework, developed at the :term:`REDS` Institute, that
      introduced the :term:`mobile entity` concept on which the SO3 capsules are
      based.

   AVZ
      Agency Virtualizer — the hypervisor on top of which the :term:`agency domain`
      (Linux) and the SO3 :term:`capsule` run. It manages the :term:`IPA`-to-:term:`PA`
      address translation stage.

   Agency domain
      The full Linux environment running next to the capsules. It hosts the
      :term:`EMISO` engine and the "critical" user interface and services.

   Capsule
      A strongly isolated :term:`SO3`-based container used to deploy a micro-service
      or application alongside Linux. A capsule is derived from the :term:`SOO`
      :term:`mobile entity` concept and is, in Docker terms, the equivalent of a
      running container.

   Mobile Entity
      Also abbreviated *ME*. The :term:`SOO` virtualization concept of a self-contained,
      migratable execution unit on which SO3 capsules are based.

   EMISO
      The engine, running in the :term:`agency domain` user space, that manages the
      lifecycle of the SO3 capsules (creation, start/stop, pause/unpause, logs). It
      exposes a subset of the Docker APIs.

   Portainer
      *Portainer CE* (Community Edition), an open-source container management tool
      used in the MICOFE framework as the :term:`COUI`. It runs on the host PC and
      talks to the :term:`EMISO` engine through a RESTful API.

   COUI
      Container Orchestration User Interface — the role played by :term:`Portainer`
      in the MICOFE framework.

   Docker
      The de-facto container platform whose APIs and concepts (image, container,
      logs) are partially mirrored by :term:`EMISO` to manage SO3 capsules.

   MUSL
      An implementation of the standard C/POSIX library designed for correctness,
      static linking, and use in embedded systems. Used as the :term:`libc` for SO3
      user-space applications.

   libc
      The standard C library providing the C/POSIX runtime to user-space
      applications. In MICOFE, the libc is :term:`MUSL`.

   libgcc
      The low-level GCC support library providing compiler helper routines (integer
      arithmetic, stack unwinding metadata, object-layout support) required by
      generated code, including :term:`C++ <RTTI>` workloads.

   LVGL
      *Light and Versatile Graphics Library* — a library for creating graphical user
      interfaces on embedded devices, used as the graphical application class for
      capsules.

   RTTI
      Runtime Type Information — C++ services such as ``dynamic_cast`` and ``typeid``.
      Optional in resource-constrained embedded systems.

   ABI
      Application Binary Interface — the low-level convention (register usage, calling
      convention, syscall numbering) that binaries must follow. SO3 uses the
      Linux-style syscall ABI expected by :term:`MUSL`.

   syscall
      System call — the interface through which a user-space application requests a
      service from the kernel. SO3 implements a subset of the Linux syscalls expected
      by :term:`MUSL`.

   ENOSYS
      The error code returned by SO3 when an application invokes a syscall that is not
      implemented, which makes missing functionality easy to diagnose in logs.

   futex
      *Fast userspace mutex* — the Linux syscall used by :term:`MUSL` for thread
      synchronization. SO3 implements the ``FUTEX_WAIT`` and ``FUTEX_WAKE`` operations.

   pthread
      The POSIX threads API. Its :term:`MUSL` implementation relies on Linux syscalls
      such as ``clone``, :term:`futex`, and ``exit``/``exit_group``.

   TLS
      Thread-Local Storage — per-thread storage used to hold thread-specific data
      (including the pthread context). On AArch64 it is held in ``tpidr_el0``.

   toolchain
      The cross-compilation tool suite (compiler, linker, runtime libraries, and
      :term:`sysroot`) built from source to produce binaries for ARM32 and ARM64 SO3
      targets.

   sysroot
      The directory tree containing the target headers and libraries against which the
      :term:`toolchain` compiles and links applications.

   binutils
      The GNU binary utilities (assembler, linker, and related tools) that are part of
      the :term:`toolchain`.

   Hypercall
      A call from a guest (Linux or a capsule) to the :term:`AVZ` hypervisor, used for
      example to share the :term:`framebuffer` address or to switch capsule focus.

   Framebuffer
      The memory region representing the whole screen content. In MICOFE its
      :term:`IPA` is remapped by :term:`AVZ` so that only the focused :term:`capsule`
      is shown on the display.

   VA
      Virtual Address — the address space seen by a user-space application, translated
      to an :term:`IPA` by the first MMU stage.

   IPA
      Intermediate Physical Address — the address space produced by the first MMU
      translation stage, sitting between the virtual (:term:`VA`) and physical
      (:term:`PA`) address spaces. Managed by Linux and the capsules.

   PA
      Physical Address — the real hardware memory address, produced by the second MMU
      translation stage managed by :term:`AVZ`.

   itb file
      The *image tree blob* used as a SO3 :term:`capsule` image. EMISO stores these
      images in the ``/root/capsule/`` folder.

   Snapshot
      A saved state of an injected :term:`capsule`. Creating a capsule snapshots the
      injected capsule; pausing a capsule snapshots its current state before shutting
      it down.

   VLOGS
      The backend/frontend driver mechanism used to forward log messages from a SO3
      :term:`capsule` to the Linux kernel, where they are stored per capsule under
      ``/var/log/soo/``.

   TrustZone
      The ARM hardware security technology leveraged, together with virtualization, to
      provide a highly secure execution environment for the capsules.

   REDS
      The *Reconfigurable & Embedded Digital Systems* institute of HEIG-VD, where the
      :term:`SOO` framework and this project were developed.
