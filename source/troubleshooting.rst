Troubleshooting
================

Camera Not Found
-----------------

**Symptom:** ``GetCameras()`` returns an empty list or times out.

Check in order:

.. code-block:: bash

   # 1. Is the NIC up and has the right IP subnet?
   ip addr show eth0

   # 2. Can you ping the camera? (use the camera's IP from Vimba X Viewer)
   ping 192.168.0.42

   # 3. Is the GigE TL registered?
   echo $GENICAM_GENTL64_PATH

   # 4. Run Vimba X Viewer to rule out software issues
   ~/VimbaX_2026-1/bin/VimbaXViewer

   # 5. Check firewall — GigE Vision uses UDP ports 3956 and dynamic ports
   sudo ufw status
   sudo ufw allow 3956/udp

Incomplete Frames / Frame Drops
---------------------------------

**Symptom:** ``GetReceiveStatus()`` returns ``VmbFrameStatusIncomplete``
or frames arrive with missing data.

.. code-block:: bash

   # Check MTU — must be 9000 for SHR
   ip link show eth0 | grep mtu

   # Check receive buffer
   sysctl net.core.rmem_max
   # Must be >= 33554432

   # Apply if missing
   sudo ip link set eth0 mtu 9000
   sudo sysctl -w net.core.rmem_max=33554432

Also check:

- Increase ``bufferCount`` from 5 to 8 or more
- Increase ``GVSPBurstSize`` to 128
- Ensure no other processes are consuming the NIC bandwidth

``VmbImageTransform`` Returns Error
-------------------------------------

**Symptom:** ``VmbImageTransform()`` returns a non-zero error code.

Common causes:

- **Width or height is odd** — Bayer formats require even dimensions.
  Check that ``Width`` and ``Height`` features return even values.
- **Wrong source pixel format** — verify with
  ``camera->GetFeatureByName("PixelFormat", f); f->GetValue(fmt);``
  and pass the exact ``VmbPixelFormat_t`` to ``VmbSetImageInfoFromPixelFormat()``.
- **Output buffer too small** — must be ``width × height × 3`` bytes for RGB8.
- **Multi-byte format is big-endian** — the library only accepts little-endian.
  Check the camera's ``PixelFormat`` vs the table in :doc:`image_transformation`.

High CPU Usage
---------------

**Symptom:** CPU pegged at 100% during capture.

- Move image processing out of ``FrameReceived()`` into a worker thread
  (see :doc:`saving_frames` — Worker Thread section)
- Enable real-time scheduling (see :doc:`system_settings`)
- Use a lower debayer quality mode: ``VmbDebayerMode2x2`` is fastest

Shutdown Crash or Hang
-----------------------

**Symptom:** Application crashes or hangs when stopping.

Always follow the shutdown sequence **in order**:

.. code-block:: cpp

   runCmd(camera, "AcquisitionStop");  // 1. Stop camera sensor
   camera->EndCapture();               // 2. Stop capture engine
   camera->FlushQueue();               // 3. Cancel pending frames
   camera->RevokeAllFrames();          // 4. Release buffers
   camera->Close();                    // 5. Close camera
   system.Shutdown();                  // 6. Unload TLs

``EndCapture()`` blocks until all ``FrameReceived()`` callbacks have
returned. If a callback is blocked (e.g. waiting on a mutex), the
application will hang here.

Vimba X Viewer Works but Code Doesn't
---------------------------------------

This usually means the Vimba X shared libraries are not on the library
path at runtime:

.. code-block:: bash

   export LD_LIBRARY_PATH=~/VimbaX_2026-1/api/lib:$LD_LIBRARY_PATH
   ./build/SHR_Capture_App

Add this export to ``~/.bashrc`` to make it permanent.

Android App & USB NCM
----------------------

**Android app shows "Disconnected" and never connects**

Check the USB NCM link first:

.. code-block:: bash

   # On V3000
   ip addr show usb0
   # Must show inet 192.168.100.1/24

   ping 192.168.100.2 -c 3
   # Android tablet must respond

If ``usb0`` is missing, the USB gadget service failed:

.. code-block:: bash

   sudo systemctl status usb-gadget
   journalctl -u usb-gadget -n 30

If the interface is up but Android doesn't respond, check that
Android has assigned ``192.168.100.2`` on the USB interface:

.. code-block:: bash

   adb shell ip addr show usb0

On some Android versions the USB network mode must be manually set:
**Settings → Network → USB → USB tethering**.

**BackendServer not running**

.. code-block:: bash

   sudo systemctl status shr-backend
   journalctl -u shr-backend -n 50

   # Test TCP port from V3000
   nc -zv 192.168.100.1 9100

**APK install fails**

.. code-block:: bash

   # Check ADB can see the tablet
   adb devices

   # If not listed, enable Developer Options on the tablet:
   # Settings → About → tap Build number 7 times
   # Settings → Developer options → USB debugging ON

   # If install rejected (version conflict)
   adb uninstall com.shrproject.cameracontrol
   adb install SHR_Camera_Android.apk

**QML UI is slow or janky on the tablet**

- Ensure **Hardware acceleration** is enabled:
  ``AndroidManifest.xml`` must have
  ``android:hardwareAccelerated="true"`` on the ``<application>`` tag
- Check the tablet meets Android 13+ requirement
- Reduce ``FrameLog`` model size from 200 to 50 entries in
  ``FrameLog.qml`` if the log is visibly causing frame drops

**Android screen goes black during acquisition**

The ``WAKE_LOCK`` permission in ``AndroidManifest.xml`` prevents
sleep, but some Android OEMs ignore it for USB-connected devices.
Add this in ``main_android.cpp``:

.. code-block:: cpp

   // Keep screen on while app is running
   // Call after QGuiApplication is created
   #if defined(Q_OS_ANDROID)
   #include <QJniObject>
   QJniObject::callStaticMethod<void>(
       "org/qtproject/qt/android/QtNative",
       "setContext",
       "(Landroid/content/Context;)V",
       QNativeInterface::QAndroidApplication::context());
   #endif

Or set in ``AndroidManifest.xml`` activity:
``android:keepScreenOn="true"``
