
.. _linux_usr:


Linux user applications (usr)
*****************************

In addition to the contents defined in the *rootfs*, additional applications can be built and deployed in the
``linux/usr/`` directory. Such applications are specific to the agency and do not belong to any external packages.

Deployment in the rootfs
========================

All applications and files which need to be deployed in the rootfs must be first installed
in the ``usr/build/deploy`` directory. To do so, the current approach is to edit
the ``usr-linux`` recipe adding the *install* command, for example:

.. code:: bash

   usr_do_install_file_root "${IB_TARGET}/build/lib/lv_port_linux/lvglsim"

This command will copy the file ``lvglsim`` to the ``usr/build/deploy`` directory.


Development of modules and deployment
=====================================

Kernel modules can also be compiled in the ``usr/module/`` directory according to the platform as defined 
in ``build/conf/local.conf`` file.

The modules are automatically deployed in the ``root/`` home directory of the target ``rootfs``.
The ``insmod`` application can then be used from the *shell* in order to load the module
into the kernel.

A module can be helpful for testing purposes, for example to test kernel functionalities.


