.. _cpp:

Integration of C++ in SO3 user space
#####################################

C++ support in SO3 user space is built as a direct extension of the C runtime
foundation described in :ref:`MUSL libc support <syscalls>`. Once the kernel
interface and startup environment are sufficiently aligned with MUSL expectations,
most of the mechanisms required by C++ applications become accessible as well,
because the C++ runtime ultimately depends on the same process startup model,
memory management primitives, thread support, and error-handling conventions.

This means that the *syscall* adaptation effort undertaken for MUSL is reused
almost entirely by C++ workloads. In practice, the project did not implement an
independent C++ execution environment; instead, it enabled C++ by making the
underlying C/POSIX runtime robust enough to support the additional language
services expected by a C++ compiler and its runtime libraries.

Main runtime requirements
*************************

Compared with plain C applications, C++ introduces several additional runtime
requirements that must be handled correctly by the loader, the linker, and the
low-level runtime support. The most visible difference is that a C++ program is
not limited to calling a single entry point and using flat procedural code: it
may rely on object construction before ``main``, destruction after program
termination, compiler-generated helper routines, and richer type-system services.

* **Constructor and destructor sections.** Sections such as ``.ctors``, ``.dtors``,
  or their modern equivalents contain initialization and finalization routines
  associated with global and static objects. They must be discovered and executed
  in the correct order during program startup and shutdown; otherwise, C++ objects
  may remain uninitialized or be destroyed incorrectly, which would make even
  simple applications unreliable.

* **Compiler support routines.** Low-level runtime libraries such as ``libgcc``
  provide helper functions for integer arithmetic, stack unwinding metadata,
  object-layout support, and other compiler-emitted constructs — even when
  exceptions are disabled. In an embedded environment, these dependencies must be
  understood and provided explicitly rather than assumed to exist implicitly as
  they would on a desktop Linux system.

* **Runtime Type Information (RTTI).** RTTI provides services such as
  ``dynamic_cast`` and ``typeid``, which are useful when software components use
  inheritance and polymorphism. In resource-constrained embedded systems, RTTI is
  often made optional, but the platform must still be able to support it correctly
  when developers decide that the application architecture benefits from it.

* **Thread-local and synchronization services.** These must behave correctly,
  especially when higher-level C++ abstractions are implemented internally on top
  of *pthread* or *libc* primitives. This links C++ support back to the MUSL
  compatibility effort: if thread creation, TLS setup, memory allocation, or
  synchronization behavior are not sufficiently aligned with Linux/MUSL
  expectations, C++ applications will fail in ways that are often difficult to
  diagnose.

Compiler and runtime integration
********************************

The project toolchain was extended so that, in addition to the C compiler and
MUSL-based runtime, it also produces the C++ compiler components and the
associated low-level support libraries. In practice, this includes the compiler
driver for C++ compilation as well as the ``libgcc`` support routines required by
generated code. The objective is not merely to compile isolated C++ files, but to
make standard cross-compilation workflows available for complete SO3 user-space
applications.

This integration has consequences both at build time and at runtime. At build
time, the toolchain must expose the expected compiler front-end, headers, startup
objects, and libraries. At runtime, the generated binaries must remain compatible
with the startup sequence, memory layout, and system-call behavior provided by
SO3. The success of the C++ integration therefore depends not only on enabling a
compiler flag, but on ensuring consistency between the compiler, the runtime
libraries, and the operating system environment.

In embedded systems, the exact feature set exposed to C++ applications must be
chosen carefully. Some advanced runtime services can increase code size, memory
usage, and startup complexity. For that reason, the project follows a pragmatic
approach in which the baseline C++ environment remains compatible with embedded
constraints while still enabling application-level abstractions that are valuable
for maintainability and software structuring. For example, RTTI can be enabled
when needed but is not mandatory for all applications, and exception handling may
be restricted or disabled depending on the target profile.

Expected benefits for applications
**********************************

Adding C++ support is important because many embedded graphical and middleware
applications benefit from stronger abstraction mechanisms than plain C alone.
Classes, namespaces, templates, and stronger type modelling help organize
medium-size and large codebases, especially when multiple services, UI components,
and communication layers must coexist in the same application.

Within MICOFE, C++ support is particularly relevant for user-space capsules that
package richer application logic on top of the MUSL runtime. It prepares the
ground for demonstrators and future products in which reusable software components,
object-oriented UI wrappers, hardware abstraction layers, and board-independent
service modules can be expressed in a more maintainable way than in plain C.

Better language-level structuring also improves software quality by making
interfaces clearer, reducing duplication, and easing the separation between
platform-specific code and portable application logic. In a project aiming at
capsule portability across multiple boards, that separation is especially valuable.

Current scope and limitations
*****************************

The current work demonstrates that the SO3 user space can host a functional C++
toolchain and execute applications that rely on the same runtime substrate as C
programs. However, this should not be interpreted as full desktop-class C++
support. The intended scope remains embedded software, with a controlled subset of
runtime features chosen according to platform constraints.

In particular, advanced features that depend on broader OS services, dynamic
loading, or very large standard-library subsystems may require additional work.
The project therefore positions C++ support as an incremental but important
milestone: enough to support real embedded applications and reusable software
components, while leaving room for future expansion if broader standard-library
coverage becomes necessary.

Position within the project
***************************

From the perspective of the overall MICOFE architecture, C++ integration is a
direct consequence of the *libc* and *syscall* compatibility effort. Once a stable
runtime, startup sequence, and build chain are available, C++ becomes a natural
extension rather than a disconnected feature. This confirms that the project has
not only improved low-level compatibility, but has also moved SO3 closer to a
practical application platform for modern embedded development.

More broadly, enabling higher-level language support on top of the validated MUSL
environment shows that the platform can host application development workflows that
are closer to current industrial practices — a key element for adoption, because
developers are more likely to use a platform that supports structured application
development rather than only low-level experimentation.
