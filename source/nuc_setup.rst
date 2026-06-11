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
   successful manual run in Part D below.

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

All three files should be present. This is **Checkpoint 2**.

----

Part C — Deploy to NUC
------------------------

From the **laptop**, copy the binary and all Vimba X runtime files to the
NUC. Run all commands from the ``shr_backend`` project directory.

**Binary and transport layer config** — deployed to ``~/shr/``
(``VmbC.xml`` must be in the same directory as the binary):

.. code-block:: bash

   rsync -avz build/SHR_Backend              lvs@192.168.1.50:~/shr/
   rsync -avz ${VIMBAX_DIR}/api/lib/VmbC.xml lvs@192.168.1.50:~/shr/

**Vimba X shared libraries** — deployed to ``~/vimba_libs/``:

.. code-block:: bash

   rsync -avz \
       ${VIMBAX_DIR}/api/lib/libVmbC.so \
       ${VIMBAX_DIR}/api/lib/libVmbCPP.so \
       ${VIMBAX_DIR}/api/lib/libVmbImageTransform.so \
       lvs@192.168.1.50:~/vimba_libs/

**GenICam runtime** — deployed to ``~/vimba_libs/GenICam/``
(required by Vimba X, must be in its own subfolder):

.. code-block:: bash

   rsync -avz ${VIMBAX_DIR}/api/lib/GenICam/ \
       lvs@192.168.1.50:~/vimba_libs/GenICam/

**Transport layer CTI files** — deployed to ``~/vimba_libs/``:

.. code-block:: bash

   rsync -avz ${VIMBAX_DIR}/cti/*.cti lvs@192.168.1.50:~/vimba_libs/

**Camera Simulator config** — deployed to ``~/vimba_libs/``
(must be alongside the ``.cti`` file):

.. code-block:: bash

   rsync -avz \
       ${VIMBAX_DIR}/cti/VimbaCameraSimulatorTL.xml \
       ${VIMBAX_DIR}/cti/VimbaCameraSimulatorTLPresets.json \
       lvs@192.168.1.50:~/vimba_libs/

.. note::
   If you have customised ``VimbaCameraSimulatorTL.xml`` or
   ``VimbaCameraSimulatorTLPresets.json`` on the laptop
   (see :doc:`laptop_setup`), those customised versions are deployed
   automatically by the commands above.

Verify the NUC layout after deploying:

.. code-block:: bash

   ssh lvs@192.168.1.50 "ls -lh ~/shr/ && ls ~/vimba_libs/"

Expected:

.. code-block:: text

   ~/shr/:
   SHR_Backend    VmbC.xml

   ~/vimba_libs/:
   GenICam/
   libVmbC.so              libVmbCPP.so           libVmbImageTransform.so
   VimbaCameraSimulatorTL.cti    VimbaCameraSimulatorTL.xml
   VimbaCameraSimulatorTLPresets.json
   VimbaGigETL.cti         VimbaUSBTL.cti
   libsv_gev_rdma_tl.cti

----

Part D — First Run on NUC
---------------------------

SSH into the NUC and run the backend manually — **not as a service yet**:

.. code-block:: bash

   ssh lvs@192.168.1.50

   cd ~/shr
   LD_LIBRARY_PATH=~/vimba_libs:~/vimba_libs/GenICam \
   GENICAM_GENTL64_PATH=~/vimba_libs \
   ./SHR_Backend --simulator --gnss-port /dev/ttyUSB0

Expected output:

.. code-block:: text

   ===========================================
    SHR Backend 1.0
    Simulator:   yes
    GNSS port:   "/dev/ttyUSB0" @ 9600
    Output dir:  "/home/lvs/frames"
    TCP port:    9100
   ===========================================
   Vimba X started
   Camera Simulator mode — enumerating...
   Cameras found: 2
     [0] "DEV_SHR-101MP" "Allied Vision Camera Simulator (SHR-sim-101MP)..."
     [1] "DEV_SHR-151MP" "Allied Vision Camera Simulator (SHR-sim-151MP)..."
   [Trigger] UDP server listening on : 9001
   [Server] Listening on TCP : 9100
   SHR Backend running.
     TCP status stream: : 9100
     UDP trigger:       :9001
     Press Ctrl+C to stop.
   [GNSS] ERROR: "Cannot open /dev/ttyUSB0: No such file or directory"
   "Camera 1: opening DEV_SHR-101MP"
   "Camera 2: opening DEV_SHR-151MP"
   "Camera 1: 11648x8742 BayerGR12"
   "Camera 1: payload 291 MB"
   "Camera 1: acquiring"
   "Camera 2: 14192x10640 BayerGR12"
   "Camera 2: payload 432 MB"
   "Camera 2: acquiring"

.. note::
   The GNSS error is expected when no receiver is connected to
   ``/dev/ttyUSB0``. The backend continues running normally.
   The ``GC_ERR_NOT_IMPLEMENTED`` warnings from the USB TL are
   also normal on Linux and can be ignored.

This is **Checkpoint 3**.

----

Part E — Stage 1 Verification
-------------------------------

With the backend running on the NUC, open a **second terminal on the
laptop** and run the following tests.

**Test 1 — TCP status stream is flowing:**

.. code-block:: bash

   nc 192.168.1.50 9100

You should see a continuous stream of single-line JSON messages at
~10 Hz. Each message is one line — keys appear in alphabetical order:

.. code-block:: text

   {"cam1":{"buf_free":5,"bw_gbs":0,"dropped":0,"fps":0,"frames":0,"geo_tagged":false,"written_gb":0},"cam2":{"buf_free":5,"bw_gbs":0,"dropped":0,"fps":0,"frames":0,"geo_tagged":false,"written_gb":0},"gnss":{"valid":false},"sync":{"delta_ms":0.8,"dropped_total":0,"total_bw_gbs":0},"type":"status"}

Note that ``"type":"status"`` appears at the **end** of each line
because Qt's ``QJsonDocument`` serialises keys alphabetically.

Press ``Ctrl+C`` to stop ``nc``.

**Test 2 — Software trigger:**

The cameras start acquiring automatically when the backend opens them.
Send a trigger directly — no separate start command needed:

.. code-block:: bash

   echo '{"type":"trigger","target":"both"}' | nc 192.168.1.50 9100

On the NUC terminal you should see:

.. code-block:: text

   "[Cam1] #1 11648x8742 geo no-fix 163ms"
   "[Cam2] #1 14192x10640 geo no-fix 161ms"

**Test 3 — Verify files written on NUC:**

.. code-block:: bash

   ssh lvs@192.168.1.50 "ls -lh ~/frames/"

Expected:

.. code-block:: text

   -rw-rw-r-- 1 lvs lvs  435 cam1_frame_00001.json
   -rw-rw-r-- 1 lvs lvs  292M cam1_frame_00001.ppm
   -rw-rw-r-- 1 lvs lvs  436 cam2_frame_00001.json
   -rw-rw-r-- 1 lvs lvs  433M cam2_frame_00001.ppm

**Test 4 — Inspect the JSON sidecar:**

.. code-block:: bash

   ssh lvs@192.168.1.50 "cat ~/frames/cam1_frame_00001.json"

Expected output:

.. code-block:: json

   {
       "camera_index": 1,
       "camera_timestamp_ns": 0,
       "capture_timestamp_utc": "2026-06-10T20:36:12.786Z",
       "frame_filename": "cam1_frame_00001.ppm",
       "frame_index": 1,
       "gnss": {
           "fix_description": "No fix",
           "fix_quality": 0,
           "valid": false
       },
       "image": {
           "height": 8742,
           "output_format": "RGB8-PPM",
           "pixel_format_raw": "BayerGR12",
           "width": 11648
       }
   }

**Test 5 — Status stream shows frame count incrementing:**

.. code-block:: bash

   nc 192.168.1.50 9100

After triggering, the ``frames`` and ``written_gb`` fields should
increment in the status stream:

.. code-block:: text

   "cam1":{"frames":1,"written_gb":0.305...}
   "cam2":{"frames":1,"written_gb":0.453...}

All four tests passing is **Stage 1 complete**.

----

Part F — Enable systemd Service
---------------------------------

Once Stage 1 passes, enable the service so the backend starts
automatically at boot. Run these commands **on the NUC**:

.. code-block:: bash

   # Update the service LD_LIBRARY_PATH to include GenICam
   sudo sed -i \
       's|LD_LIBRARY_PATH=.*|LD_LIBRARY_PATH=/home/lvs/vimba_libs:/home/lvs/vimba_libs/GenICam|' \
       /etc/systemd/system/shr-backend.service

   sudo systemctl daemon-reload
   sudo systemctl enable shr-backend
   sudo systemctl start shr-backend

   # Verify
   sudo systemctl status shr-backend
   journalctl -u shr-backend -f

For future deploys, after rebuilding on the laptop:

.. code-block:: bash

   # From the laptop
   rsync -avz build/SHR_Backend lvs@192.168.1.50:~/shr/
   ssh lvs@192.168.1.50 "sudo systemctl restart shr-backend"
