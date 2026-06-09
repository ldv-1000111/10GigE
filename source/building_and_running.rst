Building and Running
=====================

This chapter covers building and deploying both the V3000 backend daemon
and the Android APK, and verifying the complete system end-to-end.

Prerequisites Check
--------------------

.. code-block:: bash

   # V3000 — verify tools
   g++ --version              # GCC 11+ required
   cmake --version            # 3.16+ required
   ls ~/VimbaX_2026-1/api/include/VmbCPP/VmbCPP.h  # SDK present
   echo $GENICAM_GENTL64_PATH  # GigE TL registered
   ip link show usb0           # USB NCM interface up

   # Dev machine — verify Android toolchain
   adb version                # ADB installed
   ls ~/Android/Sdk/ndk/      # NDK present
   ls ~/Qt/6.8.0/android_arm64_v8a/  # Qt for Android installed

Building the V3000 Backend
---------------------------

.. code-block:: bash

   # From the project root
   cmake -S . -B build-v3000 \
     -DCMAKE_BUILD_TYPE=Release \
     -DVIMBAX_DIR=~/VimbaX_2026-1

   cmake --build build-v3000 -j$(nproc)

   # Binary is at:
   ls build-v3000/SHR_Backend

Deploy to V3000:

.. code-block:: bash

   sudo cp build-v3000/SHR_Backend /usr/local/bin/
   sudo systemctl restart shr-backend
   sudo systemctl status  shr-backend

Expected log output:

.. code-block:: text

   Jun 08 14:23:05 v3000 shr-backend[1234]: SHR Backend starting...
   Jun 08 14:23:05 v3000 shr-backend[1234]: BackendServer: listening on port 9100
   Jun 08 14:23:05 v3000 shr-backend[1234]: TriggerServer: listening on UDP port 9001
   Jun 08 14:23:06 v3000 shr-backend[1234]: Camera 1: shr411CXGE connected at 192.168.10.41
   Jun 08 14:23:06 v3000 shr-backend[1234]: Camera 2: shr411CXGE connected at 192.168.10.42
   Jun 08 14:23:06 v3000 shr-backend[1234]: GNSS: /dev/ttyUSB0 open at 9600 baud
   Jun 08 14:23:06 v3000 shr-backend[1234]: SHR Backend running — waiting for Android client on TCP :9100

Building the Android APK
--------------------------

.. code-block:: bash

   # Set environment
   export ANDROID_SDK_ROOT=~/Android/Sdk
   export ANDROID_NDK_ROOT=~/Android/Sdk/ndk/26.3.11579264
   export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64

   # Configure for Android ARM64
   cmake -S . -B build-android \
     -DCMAKE_SYSTEM_NAME=Android \
     -DCMAKE_ANDROID_ARCH_ABI=arm64-v8a \
     -DCMAKE_ANDROID_NDK=$ANDROID_NDK_ROOT \
     -DANDROID_PLATFORM=android-26 \
     -DCMAKE_PREFIX_PATH=~/Qt/6.8.0/android_arm64_v8a

   cmake --build build-android --target SHR_Camera_Android

   # Install to connected Android tablet
   adb install build-android/android-build/SHR_Camera_Android.apk

Verifying the Complete System
-------------------------------

**Step 1 — Connect hardware:**

.. code-block:: text

   USB-C cable: Android tablet → Bedrock V3000
   SFP+ 0:      SHR Camera 1  → V3000 10GbE port 0
   SFP+ 1:      SHR Camera 2  → V3000 10GbE port 1
   USB-A:       GNSS receiver → V3000 USB port

**Step 2 — Verify USB NCM link:**

.. code-block:: bash

   # On V3000
   ping 192.168.100.2 -c 3       # Android tablet should respond

   # On Android (via ADB)
   adb shell ping 192.168.100.1 -c 3   # V3000 should respond

**Step 3 — Launch the Android app:**

The app auto-connects to ``192.168.100.1:9100`` on startup. Within
2–3 seconds the title bar badges should show:

.. code-block:: text

   ⬤ Connected    ⬤ Cam 1    ⬤ Cam 2    ⬤ Acquiring

**Step 4 — Verify GNSS:**

The GNSS panel in the right column should show ``GPS 3D`` fix badge
and live coordinates within 30–60 seconds of the receiver getting a
lock. If no fix appears, check ``/dev/ttyUSB0`` is present on the V3000:

.. code-block:: bash

   ls /dev/ttyUSB*
   # Expected: /dev/ttyUSB0

**Step 5 — Trigger a test capture:**

Tap **Trigger both cameras** in the Android app. The frame log should
immediately show two new entries — one per camera — with geo-tag status.

Check frames were written to the V3000:

.. code-block:: bash

   ls -lh /mnt/frames/
   # Expected: frame_00001.ppm  frame_00001.json
   #           frame_00002.ppm  frame_00002.json  (one per camera)

   cat /mnt/frames/frame_00001.json | python3 -m json.tool

Tail the backend log for live output:

.. code-block:: bash

   journalctl -u shr-backend -f

Expected per trigger:

.. code-block:: text

   BackendServer: frame 1 cam1 — geo-tagged — 163ms
   BackendServer: frame 1 cam2 — geo-tagged — 161ms
   BackendServer: Android client status push OK

Running Without a Real Camera (Development)
--------------------------------------------

Use the Vimba X Camera Simulator and a ``socat`` virtual serial port
as described in :doc:`dev_x86_setup`:

.. code-block:: bash

   # Terminal 1 — simulate GNSS
   socat PTY,link=/tmp/ttyGNSS0,raw,echo=0 \
         PTY,link=/tmp/ttyGNSS1,raw,echo=0 &
   while true; do
     echo '$GNGGA,142311.000,4808.2292,N,01134.5674,E,1,09,0.9,524.3,M,47.8,M,,*40'
     echo '$GNRMC,142311.000,A,4808.2292,N,01134.5674,E,0.00,0.00,070626,,,A*6F'
     sleep 1
   done > /tmp/ttyGNSS0

   # Terminal 2 — run backend (update GNSS port in config)
   ./build-v3000/SHR_Backend --gnss-port /tmp/ttyGNSS1

   # Terminal 3 — test backend from command line
   echo '{"type":"trigger","target":"both"}' | nc 192.168.100.1 9100
