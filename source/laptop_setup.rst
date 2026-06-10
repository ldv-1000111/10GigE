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

Download **Vimba X 2026-1** for **Linux x86_64**. The archive is named:

.. code-block:: text

   VimbaX_Setup-2026-1-Linux64.tar.gz

Step 3 — Extract the Archive
------------------------------

.. code-block:: bash

   cd ~
   tar -xf ~/Downloads/VimbaX_Setup-2026-1-Linux64.tar.gz

This extracts to ``~/VimbaX_2026-1/``. Verify:

.. code-block:: bash

   ls ~/VimbaX_2026-1/
   # Should show: api/  bin/  cti/  doc/  README.txt

Step 4 — Install the GenTL Path
---------------------------------

This is the official Vimba X registration step. It registers the
transport layers with the system so cameras can be discovered:

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

Launch the Viewer to confirm the SDK is working:

.. code-block:: bash

   $VIMBAX_DIR/bin/VimbaXViewer

The Viewer should open and show the Camera Simulator TL with a few
default Alvium simulated cameras. This confirms the SDK and GenTL
registration are working correctly.

Step 8 — Add SHR Camera Simulator Presets
-------------------------------------------

By default the Camera Simulator only shows small Alvium-class cameras.
To simulate SHR resolutions, two files need to be edited inside
``~/VimbaX_2026-1/cti/``.

**Part 1 — Add SHR models to VimbaCameraSimulatorTLPresets.json**

Open the file:

.. code-block:: bash

   nano ~/VimbaX_2026-1/cti/VimbaCameraSimulatorTLPresets.json

The file contains a single ``"presets"`` object with named camera
entries. Add the two SHR entries **inside** that object, after the
last existing entry (``G5-2460c``). Add a comma after the closing
``}`` of ``G5-2460c`` and append:

.. code-block:: json

       "SHR-sim-101MP": {
           "width": 11648,
           "height": 8742,
           "link_speed": 1250000000,
           "throughput_limit": 1100000000,
           "sensor": "color"
       },
       "SHR-sim-151MP": {
           "width": 14192,
           "height": 10640,
           "link_speed": 1250000000,
           "throughput_limit": 1100000000,
           "sensor": "color"
       }

The end of the file should now look like:

.. code-block:: json

       "G5-2460c": {
           "width": 5328,
           "height": 4608,
           "link_speed": 1250000000,
           "throughput_limit": 599054745,
           "sensor": "color"
       },
       "SHR-sim-101MP": {
           "width": 11648,
           "height": 8742,
           "link_speed": 1250000000,
           "throughput_limit": 1100000000,
           "sensor": "color"
       },
       "SHR-sim-151MP": {
           "width": 14192,
           "height": 10640,
           "link_speed": 1250000000,
           "throughput_limit": 1100000000,
           "sensor": "color"
       }
   }
   }

.. note::
   The format used by this file is different from the Vimba X
   documentation examples. Entries use ``"sensor"`` (``"color"`` or
   ``"mono"``), ``"link_speed"``, and ``"throughput_limit"`` — not
   ``"pixel_format"`` or ``"frame_rate"``. The key name is the camera
   model name. There is no ``"name"`` field.

**Part 2 — Select SHR cameras in VimbaCameraSimulatorTL.xml**

Adding presets to the JSON only registers the model — the XML file
controls which cameras are actually instantiated. Open it:

.. code-block:: bash

   nano ~/VimbaX_2026-1/cti/VimbaCameraSimulatorTL.xml

Find the ``<Cameras>`` section and replace its entire contents with:

.. code-block:: xml

   <Cameras>
       <Camera name="SHR-101MP" preset="SHR-sim-101MP"/>
       <Camera name="SHR-151MP" preset="SHR-sim-151MP"/>
   </Cameras>

Save the file and restart the Viewer:

.. code-block:: bash

   $VIMBAX_DIR/bin/VimbaXViewer

The two SHR simulated cameras should now appear in the camera list
under **Camera Simulator TL → Camera Simulator Interface**.

.. note::
   Vimba X uninstall deletes ``VimbaCameraSimulatorTL.xml``. Save a
   copy outside the install directory to preserve your settings across
   SDK updates:

   .. code-block:: bash

      cp ~/VimbaX_2026-1/cti/VimbaCameraSimulatorTL.xml \
         ~/VimbaCameraSimulatorTL.xml.backup

This is **Checkpoint 1** — do not proceed to the build until both
SHR cameras appear in the Viewer.

Step 9 — Set Up SSH Key for NUC
---------------------------------

Passwordless SSH to the NUC is needed for the deploy step:

.. code-block:: bash

   # Generate a key if you don't have one
   ssh-keygen -t ed25519 -C "shr-dev"

   # Copy to NUC (you will be prompted for the NUC password once)
   ssh-copy-id lvs@192.168.1.50

   # Verify — should connect without a password prompt
   ssh lvs@192.168.1.50 "echo SSH OK"

Step 10 — Set Up GNSS Simulator
---------------------------------

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
