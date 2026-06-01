.. _user_guide:

User Guide
##########
   
The installation should work in any Ubuntu/Kubuntu installation superior
to ``20.04``. It is assumed that you are running an x86_64 version.

The following description is used to build the different target boards
including the emulated environment based upon QEMU.

According to the board and requirements of your configuration, all components
are not necessary such as OPTEE-OS or even U-boot if you use x86 boards.

Pre-requisites
**************

Shell
=====

The build system requires the **bash** shell.

.. warning::

   With Ubuntu 22.04, the default shell is now ``dash`` which does not
   have the same syntax as *bash*. Please have a look at 
   `this procedure <https://askubuntu.com/questions/1064773/how-can-i-make-bin-sh-point-to-bin-bash>`_ 
   to replace *dash* by *bash* 

Packages
========

The following packages need to be installed:

.. code:: bash

    sudo apt install make cmake gcc-arm-none-eabi libc-dev \
    bison flex bash patch mount device-tree-compiler \
    dosfstools u-boot-tools net-tools \
    bridge-utils iptables dnsmasq libssl-dev \
    util-linux e2fsprogs
 
Since the documentation relies on `Sphinx <https://www.sphinx-doc.org>`_, 
the python environment is required as well as some additional extensions:

.. code:: bash

   sudo apt install python3
   pip install sphinxcontrib-openapi sphinxcontrib-plantuml

If OPTEE-OS is required, the following python packages are required:

.. code:: bash

   pip3 install pycryptodome
   sudo apt install python3-pyelftools


Toolchain
=========
 
The AArch-32 (ARM 32-bit) toolchain can be installed with the following commands:

.. code-block:: shell

   $ sudo mkdir -p /opt/toolchains && cd /opt/toolchains
   # Download and extract arm-none-linux-gnueabihf toolchain (gcc v9.2.1).
   $ sudo wget https://developer.arm.com/-/media/Files/downloads/gnu-a/9.2-2019.12/binrel/gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf.tar.xz
   $ sudo tar xf gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf.tar.xz
   $ sudo rm gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf.tar.xz
   $ sudo mv gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf arm-none-linux-gnueabihf_9.2.1
   $ sudo echo 'export PATH="${PATH}:/opt/toolchains/arm-none-linux-gnueabihf_9.2.1/bin"' | sudo tee -a /etc/profile.d/02-toolchains.sh

For the 64-bit version (virt64 & RPi4), we are using the `aarch64-none-linux-gnu toolchain version 12.1.rel1 <ARM_toolchain_>`_,
which is the official ARM toolchain. 

The AARCH-64 (ARM 64-bit) used with SO3/avz is ``aarch64-none-elf-gcc``. It can be
installed with the following commands:

.. code-block:: shell

   $ sudo mkdir -p /opt/toolchains && cd /opt/toolchains
   # Download and extract arm-none-linux-gnueabihf toolchain (gcc v12.3.1).
   $ sudo wget https://developer.arm.com/-/media/Files/downloads/gnu/12.3.rel1/binrel/arm-gnu-toolchain-12.3.rel1-x86_64-aarch64-none-elf.tar.xz
   $ sudo tar xf arm-gnu-toolchain-12.3.rel1-x86_64-aarch64-none-elf.tar.xz
   $ sudo rm arm-gnu-toolchain-12.3.rel1-x86_64-aarch64-none-elf.tar.xz
   $ sudo mv arm-gnu-toolchain-12.3.rel1-x86_64-aarch64-none-elf/  aarch64-none-elf_12.3
   $ sudo echo 'export PATH="${PATH}:/opt/toolchains/aarch64-none-elf_12.3/bin"' | sudo tee -a /etc/profile.d/02-toolchains.sh


Configuration options
*********************

The main configuration of the project resides in the ``build/conf/local.conf`` file.

There is pre-defined values for all variables.

Platforms
=========

The ``IB_PLATFORM`` variable defines the target platform (also known as "machine").

The following values are possible target platforms:

+----------------+-------------------------------+
| Name           | Platform                      |
+================+===============================+
| *virt32*       | QEMU 32-bit emulated platform |
+----------------+-------------------------------+
| *virt64*       | QEMU 64-bit emulated platform |
+----------------+-------------------------------+
| *rpi4*         | Raspberry Pi 4 in 32-bit mode |
+----------------+-------------------------------+
| *rpi4_64*      | Raspberry Pi 4 in 64-bit mode |
+----------------+-------------------------------+
| *bbb*          | BeagleBone Black platform     |
+----------------+-------------------------------+
| *x86*          | x86 PC platform               |
+----------------+-------------------------------+
| *x86_qemu*     | x86 PC emulated platform      |
+----------------+-------------------------------+
| *imx8_colibri* | x86 PC emulated platform      |
+----------------+-------------------------------+

Execution of *bitbake* task
***************************

Tasks can be executed manually or automatically depending of the dependency scheme as 
defined for a specific recipe.

For manual execution, the task can be executed with the following command, 
from the ``build/`` directory:

.. code-block:: bash

   bitbake *<recipe>* -c *<task>*

where *<task>* is the name **without** the ``do_`` prefix. For example, the *do_patch* task is
executed as follows:

.. code-block:: bash

   bitbake linux -c patch

Complete building
*****************

The build system relies on *bitbake* and requires to set some environment variables.
It can be achieved with the following script:

.. code-block:: bash

   $ source env.sh
   
However, the :term:`standard script` executes this command before invoking *bitbake* commands.

The building of all components is achieved with:

.. code-block:: bash

   $ ./build.sh -a
   
The script ``build.sh`` has different options to build component individually.

Options are:

* ``-a``  Build all from scratch
* ``-f``  Create and prepare the filesystem
* ``-l``  Build Linux from scratch
* ``-r``  Build rootfs from scratch
* ``-b``  Build U-boot from scratch
* ``-u``  Build usr apps
* ``-q``  Build QEMU with custom patches (framebuffer enabled)

QEMU
****

The installation of *QEMU* depends on the necessity to have the emulated framebuffer or not.
Currently, the QEMU machine is ``virt`` and is referred as **virt32** for 32-bit and **virt64**
for 64-bit versions in *Infrabase*.

For the standard installation, QEMU can be installed via the standard ``apt-get`` command.
There are two possible versions of QEMU according to the architecture (32-/64-bit)

.. code-block:: shell

   $ sudo apt-get install qemu-system-arm      (for 32-bit version)
   $ sudo apt-get install qemu-system-aarch64  (for 64-bit version)

In the case of the patched version (with framebuffer enabled), QEMU can be built using the build system with
the following command:

.. code-block:: bash

   $ ./build.sh -q

The script will invoke the build task of the QEMU recipe.

 
The following configurations are available:

+-----------------------+-------------------------------------+
| Name                  | Platform                            |
+=======================+=====================================+
| *vexpress_defconfig*  | Basic QEMU/vExpress 32-bit platform |
+-----------------------+-------------------------------------+
| *virt64_defconfig*    | QEMU/virt 64-bit platform           |
+-----------------------+-------------------------------------+
| *rpi_4_32b_defconfig* | Raspberry Pi 4 in 32-bit mode       |
+-----------------------+-------------------------------------+
| *rpi4_64_defconfig*   | Raspberry Pi 4 in 64-bit mode       |
+-----------------------+-------------------------------------+

(The last one is a custom configuration and is to be used as replacemenent
of rpi_4_defconfig)


Root filesystem (*rootfs*)
**************************

Main root filesystem (**rootfs**)
=================================

The main root filesystem (*rootfs*) contains all application and configuration files
required by the distribution. It actually refers to user space activities.

To mount the rootfs, the following command can be executed:

.. code-block:: bash

   $ ./mount.sh rootfs

The mounting point is the directory ``fs/``.

And to unmount:

.. code-block:: bash

   $ ./umount.sh rootfs
   
Initial ramfs (initrd) filesystem
=================================


The initial rootfs filesystem, aka *ramfs* (or *initrd*) is loaded in RAM during the kernel
boot. It aims at starting user space applications dedicated to initialization; firmware loading
and mounting specific storage can be achieved at this moment.

To mount the ramfs, the following command can be executed:

.. code-block:: bash

   $ ./mount.sh ramfs

The mounting point is the directory ``fs/``.

And to unmount:

.. code-block:: bash

   $ ./umount.sh ramfs

User space applications
***********************

Custom user applications as well as kernel modules are located in
``linux/usr``.

The build system for user applications relies on *Cmake*.



.. _ARM_toolchain: https://developer.arm.com/-/media/Files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-x86_64-aarch64-none-linux-gnu.tar.xz?rev=6750d007ffbf4134b30ea58ea5bf5223&hash=6C7D2A7C9BD409C42077F203DF120385AEEBB3F5


