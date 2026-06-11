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
laptop** for the following tests. Leave the NUC terminal visible so
you can watch both sides simultaneously.

**Test 1 — TCP status stream is flowing**

Connect to the backend status stream:

.. code-block:: bash

   nc 192.168.1.50 9100

The backend pushes one JSON message per line at ~10 Hz. Each message
is a single long line — keys are in alphabetical order because Qt's
``QJsonDocument`` serialises them that way. The message ends with
``"type":"status"}``:

.. code-block:: text

   {"cam1":{"buf_free":5,"bw_gbs":0,"dropped":0,"fps":0,"frames":0,"geo_tagged":false,"written_gb":0},"cam2":{"buf_free":5,"bw_gbs":0,"dropped":0,"fps":0,"frames":0,"geo_tagged":false,"written_gb":0},"gnss":{"valid":false},"sync":{"delta_ms":0.8,"dropped_total":0,"total_bw_gbs":0},"type":"status"}

The stream continues scrolling until you press ``Ctrl+C``. Keep
``nc`` running in this terminal — you will watch the values change
during the next tests.

**Test 2 — Software trigger fires both cameras**

Open a **third terminal** on the laptop and send a trigger command.
The cameras start acquiring automatically when the backend opens them
— no separate start command is needed:

.. code-block:: bash

   echo '{"type":"trigger","target":"both"}' | nc 192.168.1.50 9100

What happens when the trigger fires:

- The NUC backend receives the JSON command on TCP :9100
- Both ``CameraWorker`` instances call ``TriggerSoftware::RunCommand()``
- The Camera Simulator generates a synthetic frame for each camera
- Each frame goes through Bayer → RGB8 debayer transform
- A ``.ppm`` image file and a ``.json`` sidecar are written to ``~/frames/``
- The backend logs to the NUC terminal:

.. code-block:: text

   [Cam1] #1 11648x8742 no-fix 163ms
   [Cam2] #1 14192x10640 no-fix 161ms

- The status stream in your second terminal updates ``written_gb``
  immediately after the files are written. Note that ``frames``
  may still show ``0`` — this is a known metrics wiring issue fixed
  in the latest build. ``written_gb`` updating confirms the frames
  were written:

.. code-block:: text

   "cam1":{...,"written_gb":0.305480448,...},"cam2":{...,"written_gb":0.45300864,...}

**Test 3 — PPM and JSON files written to NUC**

Verify the output files were created on the NUC:

.. code-block:: bash

   ssh lvs@192.168.1.50 "ls -lh ~/frames/"

Expected — one PPM and one JSON per camera per trigger:

.. code-block:: text

   -rw-rw-r-- 1 lvs lvs  435  cam1_frame_00001.json
   -rw-rw-r-- 1 lvs lvs  292M cam1_frame_00001.ppm
   -rw-rw-r-- 1 lvs lvs  436  cam2_frame_00001.json
   -rw-rw-r-- 1 lvs lvs  433M cam2_frame_00001.ppm

The PPM files are full-resolution RGB8 images:

- ``cam1``: 11648 × 8742 px = 291 MB uncompressed
- ``cam2``: 14192 × 10640 px = 432 MB uncompressed

**Test 4 — JSON sidecar structure is correct**

Inspect the metadata sidecar for Camera 1:

.. code-block:: bash

   ssh lvs@192.168.1.50 "cat ~/frames/cam1_frame_00001.json"

Expected output — all fields present, GNSS valid=false because no
receiver is connected:

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

Verify that ``frame_index``, ``capture_timestamp_utc``, ``image.width``,
``image.height``, and ``gnss.valid`` are all present. The ``gnss``
block will show ``"valid": false`` until a real GNSS receiver is
connected — this is correct behaviour.

**Test 5 — written_gb updates in status stream after trigger**

With ``nc 192.168.1.50 9100`` still running in your second terminal,
send several triggers and watch the status stream:

.. code-block:: bash

   echo '{"type":"trigger","target":"both"}' | nc 192.168.1.50 9100

After several triggers ``written_gb`` accumulates. With 3–4 triggers
per camera the values will look similar to:

.. code-block:: text

   "cam1":{...,"bw_gbs":0.0023715415019762848,...,"written_gb":1.35902592,...}
   "cam2":{...,"bw_gbs":0.0023715415019762848,...,"written_gb":1.80953856,...}
   "type":"status"}

The ``written_gb`` value confirms frames are being written to NVMe —
each trigger adds ~292 MB for cam1 and ~433 MB for cam2.

The ``bw_gbs`` value will be very low during manual triggering (around
0.002 GB/s) because it is calculated as total bytes written divided by
total time since the backend started. Manual triggers are infrequent
compared to the session duration so the average bandwidth is low. At
sustained 6 fps continuous acquisition this value will be close to
1.25 GB/s per camera.

All five tests passing is **Stage 1 complete**.

----

Part F — Enable systemd Service
---------------------------------

Once Stage 1 passes, enable the service so the backend starts
automatically at boot.

**Step 1 — Deploy the service file from the laptop:**

.. code-block:: bash

   # From the laptop
   scp shr_backend/systemd/shr-backend.service lvs@192.168.1.50:/tmp/

**Step 2 — Install and enable on the NUC:**

.. code-block:: bash

   # On the NUC
   sudo cp /tmp/shr-backend.service /etc/systemd/system/
   sudo systemctl daemon-reload
   sudo systemctl enable shr-backend
   sudo systemctl start shr-backend

**Step 3 — Verify:**

.. code-block:: bash

   sudo systemctl status shr-backend
   journalctl -u shr-backend -f

Expected status output:

.. code-block:: text

   ● shr-backend.service - SHR Camera Acquisition Backend
        Loaded: loaded (/etc/systemd/system/shr-backend.service; enabled)
        Active: active (running) since ...
      Main PID: XXXX (SHR_Backend)

**For future deploys** — after rebuilding on the laptop, redeploy the
binary and restart the service:

.. code-block:: bash

   # From the laptop
   rsync -avz build/SHR_Backend lvs@192.168.1.50:~/shr/
   ssh lvs@192.168.1.50 "sudo systemctl restart shr-backend"

.. note::
   Always deploy the service file via ``scp`` from the repo rather
   than editing it directly on the NUC. This keeps the file in sync
   with version control and avoids manual editing errors.
