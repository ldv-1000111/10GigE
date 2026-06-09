NUC — Setup, Deploy & Stage 1 Test
=====================================

This page covers the NUC13ANHi7 initial setup, building the backend on
the laptop, deploying the binary to the NUC, and verifying the complete
Stage 1 pipeline end-to-end.

Part A — NUC Initial Setup
----------------------------

Do this once after installing Ubuntu 24.04 on the NUC. Connect the NUC
to your network and SSH in from the laptop:

.. code-block:: bash

   ssh lvs@192.168.1.50

Then run the following steps **on the NUC**.

Step 1 — Create Deployment Directories
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   mkdir -p ~/shr
   mkdir -p ~/vimba_libs
   mkdir -p ~/frames

Step 2 — Install Runtime Dependencies
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The NUC only needs Qt6 runtime libraries — not the full SDK:

.. code-block:: bash

   sudo apt-get update
   sudo apt-get install -y \
       libqt6core6t64 \
       libqt6network6t64 \
       libqt6serialport6 \
       libusb-1.0-0 \
       socat \
       net-tools \
       iproute2

Step 3 — Assign Static IP
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Find the ethernet interface name:

.. code-block:: bash

   ip link show
   # Look for an interface starting with 'en' e.g. enp86s0 or eno1

Create a netplan configuration (replace ``enp86s0`` with your interface):

.. code-block:: bash

   sudo nano /etc/netplan/01-shr-static.yaml

Add the following content:

.. code-block:: yaml

   network:
     version: 2
     renderer: networkd
     ethernets:
       enp86s0:
         addresses:
           - 192.168.1.50/24
         nameservers:
           addresses: [8.8.8.8, 8.8.4.4]
         routes:
           - to: default
             via: 192.168.1.1

Apply and verify:

.. code-block:: bash

   sudo chmod 600 /etc/netplan/01-shr-static.yaml
   sudo netplan apply

   # Verify
   ip addr show enp86s0
   # Should show inet 192.168.1.50/24

Step 4 — USB Serial for GNSS
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Add your user to the ``dialout`` group so ``/dev/ttyUSB0`` is accessible
without root:

.. code-block:: bash

   sudo usermod -aG dialout lvs

   # Load USB serial drivers
   sudo modprobe cp210x
   sudo modprobe ftdi_sio

   # Make them load at boot
   echo "cp210x"   | sudo tee -a /etc/modules
   echo "ftdi_sio" | sudo tee -a /etc/modules

.. note::
   Log out and back in (or reboot) for the group change to take effect.

Step 5 — OS Tuning for Camera Acquisition
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Increase the network receive buffer for sustained high-bandwidth streams:

.. code-block:: bash

   sudo tee /etc/sysctl.d/99-shr-camera.conf << 'EOF'
   net.core.rmem_max=33554432
   net.core.wmem_max=33554432
   net.core.rmem_default=33554432
   net.core.wmem_default=33554432
   EOF

   sudo sysctl -p /etc/sysctl.d/99-shr-camera.conf

Configure real-time scheduling limits for the ``lvs`` user:

.. code-block:: bash

   sudo tee /etc/security/limits.d/shr-realtime.conf << 'EOF'
   lvs  -  rtprio    99
   lvs  -  memlock   unlimited
   lvs  soft  nice   -10
   lvs  hard  nice   -20
   EOF

   # Activate PAM limits
   grep -qF "pam_limits.so" /etc/pam.d/common-session || \
     echo "session required pam_limits.so" | sudo tee -a /etc/pam.d/common-session

Step 6 — Install systemd Service
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The service file is included in the repository. Copy it to the system:

.. code-block:: bash

   # From the laptop — copy the service file to NUC
   scp shr_backend/systemd/shr-backend.service lvs@192.168.1.50:/tmp/

   # On the NUC
   sudo cp /tmp/shr-backend.service /etc/systemd/system/
   sudo systemctl daemon-reload

.. note::
   Do **not** enable the service yet. Enable it only after the first
   successful manual run in Step D below.

Step 7 — Reboot the NUC
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   sudo reboot

----

Part B — Build on the Laptop
------------------------------

Back on the **laptop**. Make sure environment variables are set:

.. code-block:: bash

   source ~/.bashrc
   echo $VIMBAX_DIR
   # Expected: /home/lvs/VimbaX_2026-1

Build the backend:

.. code-block:: bash

   cd shr_backend

   cmake -S . -B build \
       -DCMAKE_BUILD_TYPE=Release \
       -DVIMBAX_DIR=${VIMBAX_DIR}

   cmake --build build -j$(nproc)

Verify the build output:

.. code-block:: bash

   ls -lh build/SHR_Backend
   ls -lh build/libVmbCPP.so
   ls -lh build/libVmbImageTransform.so
   ls -lh build/VimbaCameraSimulatorTL.cti

All four files should be present. This is **Checkpoint 2**.

----

Part C — Deploy to NUC
------------------------

From the **laptop**, copy the binary and Vimba X libraries to the NUC:

.. code-block:: bash

   # Binary
   rsync -avz build/SHR_Backend lvs@192.168.1.50:~/shr/

   # Vimba X shared libraries
   rsync -avz \
       build/libVmbCPP.so \
       build/libVmbImageTransform.so \
       build/VimbaCameraSimulatorTL.cti \
       lvs@192.168.1.50:~/vimba_libs/

   # Vimba X transport layer config and CTI files
   rsync -avz ${VIMBAX_DIR}/bin/VmbC.xml lvs@192.168.1.50:~/vimba_libs/

   rsync -avz ${VIMBAX_DIR}/cti/*.cti lvs@192.168.1.50:~/vimba_libs/

Verify on the NUC:

.. code-block:: bash

   ssh lvs@192.168.1.50 "ls -lh ~/shr/ ~/vimba_libs/"

----

Part D — First Run on NUC
---------------------------

SSH into the NUC and run the backend manually — **not as a service yet**:

.. code-block:: bash

   ssh lvs@192.168.1.50

   cd ~/shr
   LD_LIBRARY_PATH=~/vimba_libs \
   GENICAM_GENTL64_PATH=~/vimba_libs \
   ./SHR_Backend --simulator --gnss-port /dev/ttyUSB0

Expected output:

.. code-block:: text

   ===========================================
    SHR Backend 1.0
    Simulator:   yes
    GNSS port:   /dev/ttyUSB0 @ 9600
    Output dir:  /home/lvs/frames
    TCP port:    9100
   ===========================================
   Vimba X started
   Cameras found: 1
     [0] DEV_SimulatedCamera_... Simulated Camera
   [GNSS] Open: /dev/ttyUSB0 @ 9600
   [Cam1] 1920×1080 BayerGR12
   [Cam1] payload 6 MB
   SHR Backend running.
     TCP status stream: :9100
     UDP trigger:       :9001
     Press Ctrl+C to stop.

.. note::
   If no GNSS receiver is connected, the ``[GNSS]`` line will show an
   error — this is expected. Use ``/tmp/ttyGNSS`` from the GNSS
   simulator on the laptop instead (see :doc:`laptop_setup`).

This is **Checkpoint 3**.

----

Part E — Stage 1 Verification
-------------------------------

With the backend running on the NUC, run these tests **from the laptop**
in a separate terminal.

**Test 1 — TCP status stream is flowing:**

.. code-block:: bash

   nc 192.168.1.50 9100

You should see JSON status messages scrolling at ~10 Hz:

.. code-block:: text

   {"type":"status","cam1":{"frames":0,"fps":0.0,...},"gnss":{"valid":false},...}

Press ``Ctrl+C`` to stop.

**Test 2 — Start acquisition:**

.. code-block:: bash

   echo '{"type":"start"}' | nc 192.168.1.50 9100

Watch the NUC terminal — camera should start acquiring.

**Test 3 — Software trigger:**

.. code-block:: bash

   echo '{"type":"trigger","target":"both"}' | nc 192.168.1.50 9100

**Test 4 — Verify files written on NUC:**

.. code-block:: bash

   ssh lvs@192.168.1.50 "ls -lh ~/frames/ | head -10"

You should see PPM and JSON files:

.. code-block:: text

   cam1_frame_00001.ppm    6.2M
   cam1_frame_00001.json   2.1K

**Test 5 — Inspect the JSON sidecar:**

.. code-block:: bash

   ssh lvs@192.168.1.50 "cat ~/frames/cam1_frame_00001.json"

Verify the structure contains ``frame_index``, ``image``, and ``gnss``
fields.

All five tests passing is **Stage 1 complete**.

----

Part F — Enable systemd Service
---------------------------------

Once Stage 1 passes, enable the service so the backend starts
automatically at boot:

.. code-block:: bash

   # On the NUC
   sudo systemctl enable shr-backend
   sudo systemctl start  shr-backend

   # Verify
   sudo systemctl status shr-backend
   journalctl -u shr-backend -f

For future deploys, after rebuilding and rsyncing the binary:

.. code-block:: bash

   # From laptop
   rsync -avz build/SHR_Backend lvs@192.168.1.50:~/shr/
   ssh lvs@192.168.1.50 "sudo systemctl restart shr-backend"
