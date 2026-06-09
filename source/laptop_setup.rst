Laptop — Developer Machine Setup
==================================

This page covers everything needed on the **laptop** (Ubuntu 24.04 x86_64)
to build the SHR backend and deploy it to the NUC. You will install system
dependencies, Vimba X, and verify the Camera Simulator before writing
a single line of code.

Step 1 — Install System Dependencies
--------------------------------------

.. code-block:: bash

   sudo apt-get update
   sudo apt-get install -y \
       build-essential \
       cmake \
       ninja-build \
       qt6-base-dev \
       qt6-base-dev-tools \
       qt6-serialport-dev \
       socat \
       python3 \
       net-tools \
       iproute2 \
       libusb-1.0-0 \
       libusb-1.0-0-dev

Step 2 — Download Vimba X
---------------------------

Go to the Allied Vision download page and create a free account if you
do not have one:

https://www.alliedvision.com/en/support/software-downloads/

Download **Vimba X 2026-1** for **Linux x86_64**. The archive will be
named something like ``VimbaX_Setup-2026-1-Linux64.tar.gz``.

Step 3 — Extract the Archive
------------------------------

.. code-block:: bash

   cd ~
   tar -xf ~/Downloads/VimbaX_Setup-2026-1-Linux64.tar.gz

This extracts to ``~/VimbaX_2026-1/``. Verify:

.. code-block:: bash

   ls ~/VimbaX_2026-1/
   # Should show: api/  bin/  cti/  README.txt  ...

Step 4 — Install the GenTL Path
---------------------------------

This is the official Vimba X registration step. It registers the
transport layer with the system so cameras can be discovered:

.. code-block:: bash

   cd ~/VimbaX_2026-1/cti/
   sudo ./Install_GenTL_Path.sh

Step 5 — Reboot
-----------------

The GenTL registration requires a reboot to take effect:

.. code-block:: bash

   sudo reboot

Step 6 — Set Environment Variables
------------------------------------

After rebooting, add Vimba X to your environment. Open ``~/.bashrc``
in a text editor and add these lines at the bottom:

.. code-block:: bash

   # Vimba X 2026-1
   export VIMBAX_DIR=~/VimbaX_2026-1
   export LD_LIBRARY_PATH=${VIMBAX_DIR}/api/lib:${LD_LIBRARY_PATH:-}
   export GENICAM_GENTL64_PATH=${VIMBAX_DIR}/cti

Then activate:

.. code-block:: bash

   source ~/.bashrc

Verify the variables are set:

.. code-block:: bash

   echo $VIMBAX_DIR
   # Expected: /home/lvs/VimbaX_2026-1

   ls $VIMBAX_DIR/api/include/VmbCPP/VmbCPP.h
   # Expected: file exists

Step 7 — Verify with Vimba X Viewer
-------------------------------------

Launch the Viewer to confirm the SDK is working and the Camera
Simulator is available:

.. code-block:: bash

   $VIMBAX_DIR/bin/VimbaXViewer

The Viewer should open. In the camera list on the left, you should see
a simulated camera — something like ``DEV_SimulatedCamera_...``.

.. note::
   If no simulated camera appears, verify that
   ``$VIMBAX_DIR/cti/VimbaCameraSimulatorTL.cti`` exists and that
   ``GENICAM_GENTL64_PATH`` is set correctly.

This is **Checkpoint 1** — do not proceed to the build until the
Viewer shows the simulated camera.

Step 8 — Set Up SSH Key for NUC
---------------------------------

Passwordless SSH to the NUC is needed for the deploy step:

.. code-block:: bash

   # Generate a key if you don't have one
   ssh-keygen -t ed25519 -C "shr-dev"

   # Copy to NUC (you will be prompted for the NUC password once)
   ssh-copy-id lvs@192.168.1.50

   # Verify — should connect without a password prompt
   ssh lvs@192.168.1.50 "echo SSH OK"

Step 9 — Set Up GNSS Simulator
--------------------------------

During development the ``socat`` tool creates a virtual serial port
that feeds NMEA sentences, replacing a physical GNSS receiver.

Open a **dedicated terminal** and run:

.. code-block:: bash

   # Create a virtual serial pair
   socat PTY,link=/tmp/ttyGNSS,raw,echo=0 \
         PTY,link=/tmp/ttyGNSS_w,raw,echo=0 &

   # Stream NMEA sentences into it (Munich coordinates as example)
   while true; do
     T=$(date -u +"%H%M%S.000")
     D=$(date -u +"%d%m%y")
     printf "\$GNGGA,%s,4808.2292,N,01134.5674,E,1,09,0.9,524.3,M,47.8,M,,*40\r\n" "${T}"
     printf "\$GNRMC,%s,A,4808.2292,N,01134.5674,E,0.00,0.00,%s,,,A*6F\r\n" "${T}" "${D}"
     sleep 1
   done > /tmp/ttyGNSS_w

Leave this terminal running. The virtual port ``/tmp/ttyGNSS`` now
behaves exactly like a real ``/dev/ttyUSB0`` GNSS receiver.

To use a **real GNSS receiver** instead, plug the USB-to-RS232 adapter
into the laptop and use ``/dev/ttyUSB0`` directly — no socat needed.

.. note::
   The NMEA checksums in the socat example above are illustrative.
   The ``NmeaReader`` validates checksums — if you see no GNSS data
   in the sidecar files, use a checksum calculator to generate valid
   sentences, or use a real receiver.

The laptop is now ready. Proceed to :doc:`nuc_setup` to prepare
the NUC, then return here to build and deploy.
