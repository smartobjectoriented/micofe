.. _introduction:

Introduction
############

``MICOFE`` (*Micro-Container for Edge Computing*) aims to provide a lightweight
micro-container environment based on Arm64 virtualization and the *SO3* operating
system. SO3 is a lightweight OS that supports key Linux-like features such as
user/kernel separation, memory paging, and multithreading.

This environment is well suited to provide developers with a strongly isolated
execution environment where full-featured applications can run alongside Linux.

MICOFE targeted several objectives, including:

* Full support for the :ref:`MUSL libc library <syscalls>`
* :ref:`C++ <cpp>` and Rust support
* Support for :ref:`LVGL-based graphical applications <lvgl>`

All objectives were achieved except Rust support, which could not be completed
due to time constraints.

Acknowledgements
****************

We sincerely appreciate the *Hasler Foundation* for making this ambitious project
possible and for helping us secure new projects with industrial partners.

We also extend our heartfelt thanks to Clément Dieperink, Jean-Pierre Miceli, and
Prof. Daniel Rossier from the *REDS Institute* of *HEIG-VD* for their invaluable
contributions to this project.
