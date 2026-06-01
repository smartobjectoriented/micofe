
.. _build_system:

Build System
############

This chapter gives an overview of the Infrabase build system.
The build commands are given in the :ref:`user guide chapter <user_guide>`.

The ``Infrabase`` build system relies on various scripts and methods, but is mainly
driven by ``bitbake`` receipes.

We call ``standard scripts`` those used so far as ``./build.sh``, ``./deploy.sh``, ``./mount.sh``, etc.

The main concepts of *bitbake* are: ``layers``, ``configurations``, ``classes``, ``recipes`` and ``tasks``.

A layer can be defined as a collection of *configurations*, *classes* and *recipes* associated to
a component. It describes the overall build process with the different tasks belonging to recipes.

Classes are configuration-independent functionalities/tasks and can have hierarchies (a base class
can be inherited by other classes).

Receipes contain rules to be executed for a specific application or component.

Tasks are defined in classes or receipes and are the core functions managed by the build system.
Tasks are either (**bash**) shell script or **python** functions

Configurations are everyhwere, but the ``local.conf`` file located in ``build/conf/`` directory contains the
general configuration. Typically, this file contains the definition of ``PLATFORM``
Configurations contain definition of *bitbake* variables that can be used by all recipes of the build system.


Bitbake environment
*******************

Infrabase build system relies on `Bitbake <https://docs.yoctoproject.org/bitbake>`_ which is the
underlying build system used by Yocto. The overall architecture is depicted on the figure below.

.. figure:: /img/Infrabase-Build_System.drawio.png
   :align: center

   Differences between Yocto and Infrabase in the build system architecture

While Yocto is mainly distribution oriented (and Poky is the distribution reference on top of Yocto), Infrabase
enables the building of various distributions, for example based on **buildroot** or **debootstrap**.

**Open-embedded** is constituted of various meta files which enhance the *bitbake* build system and it
has to be considered as part of the build system.

Infrabase build system
**********************

In *Infrabase*, *bitbake* and some small parts of *openembedded* are used to build 
the initial environment and to reconciliate patches of components after some modifications of source code. 

After the initial clone of the repository, *bitbake* allows to build all components by fetching the
code from the original location and applying related patchsets.

The updates of patches following some modifications of the repository are part of :ref:`the development flow <dev_flow>`.

We differ *bitbake* script from :term:`standard script`. During the development, the standard scripts 
available at the root directory and subdirectory (like *rootfs/* or *linux/usr*) should be used.

Build system directory organization
***********************************

All build system files (except the standard scripts) are located in the ``build/`` directory.

.. warning::
   
   Do not erase the ``build/`` directory. It is not automatically generated but
   is stored *git*.

Actually, *bitbake* creates a ``tmp/`` subdirectory within ``build/``. If a complete re-build is required,
you can delete *tmp/* at any time.

.. figure:: /img/Infrabase-IB_Architecture.drawio.png
   :align: center

   Build system directory organization as stored in git

In *bitbake*, a layer corresponds to a ``meta`` directory entry. For example, the *meta/* directory is
a generic layer which is used by all other layers. 
 
We focus on the **meta-linux** layer as an example. 
 
Directory ``conf/``
===================

This directory is general to the build system and defines the main configuration for *bitbake* (in *bitbake.conf*)
and for the project (in *local.conf*). 

Each new layer must be added in *bitbake.conf* in order to tell *bitbake* to consider the recipes describes in this layer.

In ``local.conf``, most variables are specific to *infrabase*, like:
 
   - IB_PLATFORM
   - IB_STORAGE
   - IB_TOOLCHAIN
   - *etc.*

Directory ``meta-linux/classes``
================================

It defines the generic tasks/functions that are used by the recipe, like ``do_configure`` and ``do_build`` (two
examples of tasks)

Directory ``meta-linux/conf``
=============================

Each layer has a very similar file called ``layer.conf`` which tell *bitbake* further information
about the dependencies between layers and their priorities in the build process. Currently,
all layers are processed with the same priority (4).

Directory ``recipes-*``
=======================

These are the recipes of the layer. In most cases, there is one recipe by layer (except for *rootfs*) which 
describes how to build the target component associated to the layer. Of course, depending on the number
of releases/versions, there can be several recipes as well.

Each recipe may have several subdirectories. Typically, a directory with the name of the component (*linux*) 
which contains the recipe files and a ``files/`` directory which contains additional files like patches.

The recipe
----------

The configuration and requirements of a recipe is given in a file with the ``.bb`` extension (for
example ``linux-5.10.bb`` in our case).

Patchset
--------

A *patchset* is a collection pf patches which are processed during the build, with the ``do_patch`` tasks.
In *Infrabase*, the list of patches to be applied is contained in a file with ``.inc`` extension within
the *files/* directory. And the list of *.inc* files to be considered in the recipe is described in the 
recipe file (*linux-5.10.bb*).

Directory ``tmp``
=================

*Bitbake* automatically creates a ``tmp/`` directory in *build/* for his management and project-related
files. The figure above shows the contents of this directory.

.. figure:: /img/Infrabase-Folders_tmp.drawio.png
   :align: center

   Directory tree of the *tmp/* directory in *build/*

Once a task is executed successfully, a stamp file (0 byte) is created so that *bitbake* will not re-execute

.. note::

   Note that standard scripts remove the stamp files associated to the component to be re-built.
   Only the *bsp* recipe does not delete the stamps for individual components except the one
   corresponding to itself. 


Infrabase Basic workflow
************************

The initial build can be achieved by meas of the ``./build.sh -a`` command. It will fetch, patch, prepare
the environment and build everything (kernel, rootfs, apps, etc.).

In the build process, there is a particular task called ``do_attach_infrabase`` which perform a copy
of source code in the root environment of *infrabase*. Hence, the development can be done independently
of the ``tmp/`` directory managed by *bitbake*.

Therefore, the development is done on the source code related to the branch while the original files
are not modified (in *tmp/work/*) directories.

This will allow the developers to perform a *diff* (using the ``do_updiff`` task) which will generate
the patches for the differences.

.. warning::
   
   It has to be noted that the generated patches are issued from the difference between the local
   files and the original **patched** files. This leads to an incremental patching process.
   The *diff* process is always done against the directory which is stored in ``tmp/work/<component>/`` 


Building a patchset
===================

Following a development sprint, patchsets have to be (re-)generated in order to keep track of the
code evolution. This is achieved by means of the ``do_updiff`` task. It has to be executed in the
``build`` directory. For example, if we do changes in linux, the patchset will be generated
with the following command:

.. code-block:: bash

   ~/infrabase/build$ bitbake linux -c updiff

As result, the patchset is generated in ``build/meta-linux/recipes-linux/linux/files`` directory with
the file ``000x-linux-5.10-r0-patches.inc`` and its associated directory called ``000x-linux-5.10-r0``
in which the set of patches is located.

The prefix is made of four digits and is incremented at each patchset generation.

Based on these two elements, the patchset can be manually worked out.

To include a patchset in a recipe, the recipe file has to include the following lines:

.. code-block:: bash

   FILESPATH:prepend: := "${THISDIR}/files/0001-${PF}:" 
   
   require files/0001-${PF}-patches.inc

Each recipe can have one or several patchsets according to the patch organization, and 
the first patchset should be called with prefix ``0001-``

Infrabase recipes
*****************

General comments
================

If a task requires ``sudo`` to execute a command, it has to be configured so that no password is required.
To do this, the following entry in the file ``/etc/sudoers`` should be added:

.. code-block:: bash

   <user>  ALL=(ALL) NOPASSWD: ALL

Currently, the use of *fakeroot* commands, that could avoid setting no password with sudo, 
does not allow to use ``losetup`` correctly.


.. warning::

   Using ``sudo`` in a task involves to set the attribute ``network`` of this task to ``"1"``. For example:
   *do_init_storage[network] = "1"*
   
   
   
