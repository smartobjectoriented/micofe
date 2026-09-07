.. _portainer:

Portainer
#########

*Portainer CE* (Community Edition) is an open-source container management tool
that simplifies the deployment, management, and monitoring of Docker containers
and containerized applications. It provides a user-friendly web-based interface
that allows users to interact with Docker and manage containers, images, networks,
and volumes without needing to use complex command-line tools.

In the SOO framework, `Portainer Server CE` is used as the Container Orchestration
Use User Interface (COUI).

Installation
************

Portainer Server runs as lightweight Docker containers on a Docker engine. It means
docker must be installed on the Host PC.

* Creation of a volume that Portainer Server use to store it database:

.. code-block:: shell

    $ docker volume create portainer_data

* Download and install Portainer Server container:

The following command install Portainer server version 3.33.1.

.. code-block:: shell

    $ docker run -d -p 8000:8000 -p 9443:9443 --name portainer --restart=always \
      -v /var/run/docker.sock:/var/run/docker.sock \
      -v portainer_data:/data \
      --add-host=host.docker.internal:host-gateway \
      portainer/portainer-ce:2.33.1

``docker ps`` command can be used to check if the Portainer server is running

.. note::

	The ``--add-host`` line is what lets Portainer reach an EMISO engine
	running *outside* its own container — in particular one running in a
	QEMU guest on the same PC. Inside the Portainer container ``localhost``
	is the container itself, never the host. See :ref:`portainer-qemu`.

To log-in, open a web browser and to to:

.. code-block:: shell

    https://localhost:9443

Initial setup
*************

Once the Portainer Server has been deployed, and you have navigated to the instance's
URL, you are ready for the initial setup.

When connecting to the Portainer server for the first time, an initial user account
must be created. This account will serve as the administrator.
The default username is ``admin``, and the password must be at least 12 characters
long.

Once the admin user has been created, the Environment Wizard will automatically
launch. The wizard will help get you started with Portainer.

The installation process automatically detects your local environment and sets it
up for you. If you want to add additional environments to manage with this Portainer
instance, click Add Environments. Otherwise, click Get Started to start using
Portainer!

Creation of an environment
**************************

In short, an environment in Portainer represents a SOO mobile entity.

* Select "environment"
* Click "+ Add environment"
* Select "Docker Standalone" --> click "Start Wizard"
    * Select "API"
    * Provide a name to the environment
    * Set the API + port (default port is 2375)
    * no TLS

The *Environment address* is the address at which the Portainer Server reaches
the EMISO engine, i.e. the Smart Object's address seen from the Portainer
container — ``<smart-object-ip>:2375`` for a real board on the LAN.

.. _portainer-qemu:

Reaching an EMISO engine running in QEMU
****************************************

When the agency runs under QEMU (``st.sh``), it uses slirp user-mode
networking: the guest always gets the address **10.0.2.15**, which is internal
to the QEMU process and routable neither from the host nor from the LAN. The
engine is reached through the port forward set up by ``st.sh``:

.. code-block:: text

	host :2375  ->  guest :2375   (EMISO engine)
	host :2222  ->  guest :22     (SSH, if the rootfs ships a server)

``st.sh`` prints both port numbers at start-up. They are offset by the number
of QEMU instances already running (``:2376``, ``:2377``, … for the second and
third guest), and further forwards can be added without editing the script:

.. code-block:: shell

	$ IB_QEMU_HOSTFWD="tcp::9000-:9000" st.sh

The engine itself is not started automatically on the busybox agency — from the
agency console:

.. code-block:: shell

	# /root/emiso_engine &
	Server started on port 2375

Then check the forward from the host, and register the environment:

.. code-block:: shell

	$ curl http://localhost:2375/_ping        # -> OK

The environment address depends on where the Portainer Server itself runs:

* as a Docker container on the host PC (the installation command above):
  ``host.docker.internal:2375`` — or the ``docker0`` gateway,
  ``172.17.0.1:2375``. **Not** ``localhost:2375``, which inside the container
  points at the container itself.
* natively on the host PC: ``localhost:2375``
* on another PC of the LAN: ``<host-pc-ip>:2375``

The forwarded ports are bound on ``0.0.0.0``, so the last case works without
any further change — it is the *host PC* that is visible on the LAN, not the
guest.

Creation of a SO3 Capsule
*************************

* Select the environment on which to create the container
* Select "Containers" --> click "+ Add container"
    * Give a name at the container
    * Provide the image name in image field (``<CAPSULE NAME>)``
    * Click on "Advanced mode" to select the "Simple mode"
    * Disable "Always pull the image" button
    * Click on "Deploy the container"
