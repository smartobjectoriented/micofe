.. _syscalls:

Alignment of *syscalls*
#######################

*SO3* use a customized version of the *MUSL* library to provide *libc* implementation to user space applications.
*MUSL* is an implementation of the standard *C/POSIX* focus for embedded devices.

The main modifications apported to the library for *SO3* are:

  * Custom *syscalls* numbers
  * Simplified thread handling, as well as custom *syscall* for creation, exiting and joining
  * Custom *syscall* for mutex
  * Most function and wrapper around *syscalls* aren't compiled as not supported by *SO3*
  * Custom program entry initialization
  * ``errno`` variable handling

Those modifications were made to simplify implementation of the *SO3* kernel as most feature from available
in Linux aren't needed.

However, the *syscall* ABI is already the same as the one expected by MUSL. This means that the *syscall* number is set
into the register ``x8`` (AArch64) or ``r7`` (Arm32), arguments are passed on registers ``x0`` to ``x5``
or ``r0`` to ``r5`` and return value is set to ``x0`` or ``r0``.

Also, the majority of implemented *syscalls* have nearly the same arguments and behavior as the *Linux* ones.

Analysis of MUSL requirements
*****************************

In order to fully support the MUSL library, it will be needed to align user space applications and SO3 kernel
to natively works with MUSL. As the goal is to have an unmodified version of MUSL, no implementation is required
on the library itself. Only the userspace build system needs to be rework to use the full library via a toolchain.

*MUSL* has two externals *API*. The one presented to user application, which is the standard *libc* functions
(``printf``, ...) and the one presented by the kernel via syscalls, which is based on *Linux*.

The majority of the API presented to user application was left untouched by the modification made to MUSL,
and so, little adaptation will be required on application to account for those changes.
On the other hand, the kernel will need to be aligned with Linux syscall to support MUSL.

Analysis of syscalls
====================

Linux provides more than 400 different syscalls, and not all are available on all CPU architecture. In example,
``fork`` is available on Arm32, but not on AArch64 which use ``clone`` instead. SO3 supports only 50 syscalls,
which is not a lot and would require a lot of work to implement all missing one. Luckily, most of them aren't
required to run C application, or even from other languages.

The full list of syscall can be found in ``include/linux/syscalls.h`` of Linux source code, it's all functions with
the prefix ``sys_``.

Syscalls can be divided in multiple categories:

  * File (``open``, ``read``, ...)
  * File system (``mount``, ``stat``, ...)
  * File permission (``chmod``, ``chown``, ...)
  * Process/thread (``fork``, ``exit``, ...)
  * Interprocess (``pipe``, ``kill``, ...)
  * Network (``socket``, ``listen``, ...)
  * And other

In SO3, not all categories are wanted. For example, there is no user/group support, so everything related to file
permission, user and group will not be needed.

Also, some syscall provides the same functionality but with different parameters. Like ``stat``, ``fstat``,
``lstat`` and ``fstatat``, all give the same information about a file, but with different set of arguments. They can
easily be implemented with a common function and by converting arguments into equivalent values. If a syscall is
unavailable for a CPU architecture, there will have another one equivalent to it.

Different numbers are assigned to all syscalls, but this number can be different between CPU architecture
(``exit`` is ``1`` on Arm32 and ``93`` on AArch64).

Application runtime context
===========================

At startup, the library expect the following information to be present on the stack (before the stack pointer):

  * ``argc``: Arguments count to be passed to ``main``
  * ``argv[]``: Arguments array with ``argc`` elements to be passed to ``main``, ``NULL`` terminated
  * ``envp[]``: Array with all environment variables, ``NULL`` terminated
  * ``aux[]``: Auxiliary array with system (page size, hardware capabilities, ...)
               and application information (elf program header), ``NULL`` terminated.

For the ``aux`` array, this is a list of an id followed by its value. A full list of id can be found in
``include/uapi/linux/auxvec.h`` from Linux. On SO3, most of them will not be useful as they are either not used at all
by *MUSL*, or used for dynamic linking.

Then, a thread local storage (TLS) is set by *MUSL*, which is used to store thread specific information
(``pthread`` context). On AArch64, the TLS address it directly set into the CPU register ``tpidr_el0``. For Arm32,
it will be set into the coprocessor register c13-c0-3, which must be modified with privileged mode and require a
special syscall with number ``0xf0005`` for that. On both arch, this register needs to be saved and restore correctly
when a switch of thread occurs.

Difference in SO3 implementation
********************************

As mentioned before, SO3 implements only a small subset of all syscalls available in Linux,
and they aren't always exactly the same. There are multiple cases:

  * Arguments mismatch between Linux and SO3.

    * Missing arguments, like flags or optional secondary return value.

      * Can be ignored at first by returning an error and/or printing a warning.
      * In any case, the user applications don't use them for now and so will not see the difference.

    * Different types (``int`` instead of ``long``)
    * Different structure

      * Generally, a missing value or different type of value.

  * Syscall isn't available on AArch64

    * Equivalent ones are available with more arguments (like ``openat`` for ``open``).
    * *MUSL* implement wrapper around them, calling with default values for missing arguments.
      Then, the new syscalls can only check those default values and call the old syscall.

  * There is an 32 bits and 64 bits version of the syscall (for Arm32, which can use both)

    * Like ``gettimeofday`` and ``gettimeofday_time32``
    * Contains the same set of arguments between the two versions, but with their size accordingly.
      The 64 bits versions can be implemented, with the 32 bits one calling it and converting the datas.

  * The SO3 syscall doesn't exist in Linux (everything thread related)

    * More detail are given in :ref:`thread-execve`


However, in general the behavior of those syscalls are the same as in Linux, meaning that no big implementation change
will be required.

SO3 used the same set of syscall numbers between Arm32 and AArch64. A more dynamic system is required to account for
those differences, as well as to account for syscalls that are only available on one architecture.

``errno`` handling
==================

Another difference between Linux and SO3 is how the variable ``errno`` is handled. That variable is used in user space
to get an error code when a problem occurs in a syscall (missing files, no more memory, ...). A set of error code is
defined to give *detailed* information about the generated problem (``EINVAL``, ...). On Linux, the negative value of
those codes is returned by syscall if an error occurred, but then the libc will generally set the global variable
``errno`` to the corresponding code and return ``-1`` to the application.

In SO3, a shared variable between user and kernel space is used for ``errno`` which is then set by the kernel on errors.
This needs to be changed to support MUSL.

.. _thread-execve:

Threads and execve
==================

On SO3, custom syscall are implemented to create, join and exit a thread. However, pthread implementation of MUSL
will use ``clone`` (generic ``fork``), ``futex`` and ``exit`` for those.

The custom syscall to create a thread takes the thread function and arguments and the kernel will directly return
to it when the thread start. On the other hand, ``clone`` works the same way as ``fork``, meaning that the child thread
must return to the current user address with a return value of 0 and the function will be called by userspace
(by MUSL wrapper). Actually, both ``fork`` and ``clone`` can use the same function with different flags to know if
the process must be duplicated or not, ...

Also, ``clone`` has a stack and tls argument to set the address of those two registers for the child thread.
On SO3, the user stack is allocated by the kernel and so require to be changed.

The ``futex`` syscall is missing from SO3 and will then need to be implemented. However, only the commands to wait and
wake are required for MUSL and so other ones will not be implemented.

The syscall ``exit`` is actually already existing, but implements the ``exit_group`` instead. This will need to be
renamed and ``exit`` to be implemented. The difference between those two syscalls is that ``exit`` only finish one
thread and ``exit_group`` finish the whole thread group, which corresponds to a process in Linux.

Missing Syscall for MUSL
========================

The following list of syscalls are necessary for application to run with MUSL, but are missing from SO3:

  * ``readv/writev``: Used for ``printf`` or read/write on file opened with ``fopen``
  * ``mmap`` with anonymous flag: Used to allocate new heap memory if needed.
  * ``exit_group``: Used to exit the process and all its thread.
  * ``futex``: Used for thread locking (mutex).
  * ``set_tid_address``: Used in addition with ``futex`` for thread joining.
  * ``rt_sigprocmask``: Allow to temporarily block some signal to handle them later.

Implementation changes
**********************

SO3 kernel has been modified to account for the difference listed above making it compatible with basic application
using MUSL as a libc.

Existing Syscalls have been adapted to account for new arguments and required missing one were added.
Not all syscalls possible are implemented. The kernel will print a message if the requested syscall isn't implemented
with its number and return the error code `ENOSYS`, making things easier to debug when running new application.
As well, not all flags for existing syscalls are implemented which will also leads in a message to be printed and
depending on cases, the syscall will just ignore the flags and continue or return an error.

To simplify implementation of new syscalls in the kernel, the macros ``SYSCALL_DECLARE`` and ``SYSCALL_DEFINE``
have been added to declare and define the syscall function with an auto-generated wrapper to call it with the
correctly cast arguments. Those macro are based on the one in *Linux*.
In addition, an array mapping a number to it corresponding syscall function is now used. It's generated at build time
by a script that convert into a C array declaration, the file ``syscall.tbl``, listing all available syscall, and
the file ``syscall.h.in``, list of all number available in Linux taken from MUSL sources and arch dependent.
This script is also inspired from Linux.

In fine, all syscall available in SO3 are now listed in ``syscall.tbl``. Here is a summary of them with some limitations:

  * File operation, ``open[at], close, write[v], read[v], [_l]lseek, ioctl, [new][fs]tatat[64], dup[2-3]``

    * The mode for ``open`` is ignored.
    * Only current working directory is supported by ``openat`` and ``fstatat``
    * ``dup3`` flags aren't supported.

  * Memory mapping, ``mmap, mmap2``:

    * Except ``MAP_ANONYMOUS``, all flags are ignored.

  * Process/threads, ``fork, clone, execve, wait4, exit, exit_group, set_tid_address, futex``

    * ``clone`` doesn't support flags to create new process
    * ``clone`` support only a given set of flags to create new thread (based on what is used by MUSL)
    * Only ``FUTEX_WAKE`` and ``FUTEX_WAIT`` operations are supported for ``futex``
    * Timeout for ``futex`` isn't implemented.

  * Signals, ``rt_sigaction, kill, [rt_]sigreturn, sigreturn, rt_sigprocmask``
  * IPC, ``pipe, pipe2``

    * ``pipe2`` flags aren't supported.

  * Time, ``nanosleep, gettimeofday[_time32], clock_gettime[32]``
  * Network, ``socket, connect, bind, ...``
