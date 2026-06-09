API Quick Reference
====================

This page summarises the Vimba X C++ API functions and Image Transform Library
functions used in this tutorial.

VmbSystem
----------

.. list-table::
   :header-rows: 1
   :widths: 45 55

   * - Function
     - Purpose
   * - ``VmbSystem::GetInstance()``
     - Get the singleton system reference
   * - ``system.Startup()``
     - Initialise the API and load transport layers
   * - ``system.Shutdown()``
     - Unload TLs and destroy all API objects
   * - ``system.QueryVersion(VmbVersionInfo_t&)``
     - Get SDK version (can be called before Startup)
   * - ``system.GetCameras(CameraPtrVector&)``
     - Enumerate all connected cameras
   * - ``system.OpenCameraByID(id, mode, CameraPtr&)``
     - Open a camera by IP, MAC, or serial number
   * - ``system.RegisterCameraListObserver(...)``
     - Register callback for camera connect/disconnect events
   * - ``system.GetInterfaces(InterfacePtrVector&)``
     - List all detected NICs / frame grabbers

Camera
-------

.. list-table::
   :header-rows: 1
   :widths: 45 55

   * - Function
     - Purpose
   * - ``camera->Open(VmbAccessModeFull)``
     - Open camera for read/write access
   * - ``camera->Close()``
     - Close the camera
   * - ``camera->GetName(std::string&)``
     - Human-readable camera name
   * - ``camera->GetModel(std::string&)``
     - Model identifier
   * - ``camera->GetSerialNumber(std::string&)``
     - Serial number
   * - ``camera->GetFeatureByName(name, FeaturePtr&)``
     - Access a named feature
   * - ``camera->GetFeatures(FeaturePtrVector&)``
     - List all features
   * - ``camera->GetPayloadSize(VmbUint32_t&)``
     - Size in bytes of one frame buffer
   * - ``camera->AnnounceFrame(FramePtr)``
     - Hand a frame buffer to the API
   * - ``camera->StartCapture()``
     - Start the host-side capture engine
   * - ``camera->QueueFrame(FramePtr)``
     - Queue a buffer to receive the next frame
   * - ``camera->EndCapture()``
     - Stop the capture engine (blocks until all callbacks return)
   * - ``camera->FlushQueue()``
     - Cancel all queued frames
   * - ``camera->RevokeAllFrames()``
     - Release all announced frame buffers
   * - ``camera->StartContinuousImageAcquisition(n, observer)``
     - Convenience: allocate n buffers and start acquisition
   * - ``camera->StopContinuousImageAcquisition()``
     - Convenience: stop and release all buffers

Feature
--------

.. list-table::
   :header-rows: 1
   :widths: 45 55

   * - Function
     - Purpose
   * - ``feature->GetValue(VmbInt64_t&)``
     - Read integer or enum-as-int value
   * - ``feature->SetValue(VmbInt64_t)``
     - Write integer value
   * - ``feature->GetValue(std::string&)``
     - Read enum or string value
   * - ``feature->SetValue(const char*)``
     - Write enum or string value
   * - ``feature->GetValue(bool&)``
     - Read boolean value
   * - ``feature->SetValue(bool)``
     - Write boolean value
   * - ``feature->RunCommand()``
     - Execute a command feature
   * - ``feature->GetRange(min, max)``
     - Get valid range for integer/float features
   * - ``feature->IsReadable(bool&)``
     - Check if readable in current state
   * - ``feature->IsWritable(bool&)``
     - Check if writable in current state

Image Transform Library
------------------------

.. list-table::
   :header-rows: 1
   :widths: 55 45

   * - Function
     - Purpose
   * - ``VmbSetImageInfoFromPixelFormat(fmt, w, h, img)``
     - Describe source image from pixel format enum
   * - ``VmbSetImageInfoFromInputImage(src, layout, bpp, dst)``
     - Describe destination from source image
   * - ``VmbSetImageInfoFromInputParameters(fmt, w, h, layout, bpp, img)``
     - Describe image from explicit parameters
   * - ``VmbSetDebayerMode(mode, info)``
     - Set debayer algorithm in a VmbTransformInfo
   * - ``VmbSetColorCorrectionMatrix3x3(mat, info)``
     - Set 3×3 colour correction matrix in a VmbTransformInfo
   * - ``VmbImageTransform(src, dst, params, count)``
     - Execute the transformation

Common Feature Names
---------------------

.. list-table::
   :header-rows: 1
   :widths: 30 20 50

   * - Feature Name
     - Type
     - Description
   * - ``PixelFormat``
     - Enum
     - ``BayerGR12``, ``Mono12``, ``BayerGR8``, etc.
   * - ``Width``
     - Integer
     - Image width in pixels (writable)
   * - ``Height``
     - Integer
     - Image height in pixels (writable)
   * - ``WidthMax``
     - Integer
     - Maximum supported width (read-only)
   * - ``HeightMax``
     - Integer
     - Maximum supported height (read-only)
   * - ``AcquisitionMode``
     - Enum
     - ``Continuous``, ``SingleFrame``, ``MultiFrame``
   * - ``AcquisitionStart``
     - Command
     - Start sensor acquisition
   * - ``AcquisitionStop``
     - Command
     - Stop sensor acquisition
   * - ``GVSPBurstSize``
     - Integer
     - GigE packet burst size (set to 64+ for 10GigE)
   * - ``ExposureTime``
     - Float
     - Exposure time in microseconds
   * - ``Gain``
     - Float
     - Analogue gain in dB
   * - ``TriggerSelector``
     - Enum
     - Select trigger event (e.g. ``FrameStart``)
   * - ``TriggerSource``
     - Enum
     - Trigger source (``Line1``, ``Software``, etc.)
   * - ``TriggerMode``
     - Enum
     - ``On`` / ``Off``

Error Codes
-----------

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Code
     - Meaning
   * - ``VmbErrorSuccess``
     - Operation succeeded
   * - ``VmbErrorNotFound``
     - Camera or feature not found
   * - ``VmbErrorBadParameter``
     - Invalid parameter (NULL pointer, wrong size, etc.)
   * - ``VmbErrorNotImplemented``
     - Transformation not supported between these formats
   * - ``VmbErrorStructSize``
     - ``VmbImage.Size`` member not set correctly
   * - ``VmbErrorApiNotStarted``
     - ``VmbSystem::Startup()`` not called
   * - ``VmbErrorInvalidAccess``
     - Camera not open or wrong access mode

----

BackendClient — Android QML Properties
----------------------------------------

All properties are exposed via ``Q_PROPERTY`` and notify ``statusUpdated()``.
In QML, access them as ``backend.propertyName`` — bindings update automatically.

.. list-table::
   :header-rows: 1
   :widths: 30 15 55

   * - Property
     - Type
     - Description
   * - ``cam1Frames``
     - int
     - Total frames captured by Camera 1
   * - ``cam1Fps``
     - double
     - Current frame rate, Camera 1
   * - ``cam1Written``
     - double
     - GB written to NVMe, Camera 1
   * - ``cam1Bw``
     - double
     - Current 10GbE bandwidth GB/s, Camera 1
   * - ``cam1Dropped``
     - int
     - Dropped frames, Camera 1 (0 = healthy)
   * - ``cam1BufFree``
     - int
     - Free Vimba X frame buffers, Camera 1
   * - ``cam1GeoTag``
     - bool
     - Last frame was geo-tagged
   * - ``cam2*``
     - —
     - Same set for Camera 2
   * - ``gnssValid``
     - bool
     - GNSS fix is active
   * - ``gnssFix``
     - QString
     - Fix description e.g. "GPS 3D"
   * - ``gnssLat``
     - double
     - Latitude, decimal degrees
   * - ``gnssLon``
     - double
     - Longitude, decimal degrees
   * - ``gnssAlt``
     - double
     - Altitude above MSL, metres
   * - ``gnssSats``
     - int
     - Satellites in use
   * - ``gnssHdop``
     - double
     - Horizontal dilution of precision
   * - ``gnssUtc``
     - QString
     - UTC time string from GNSS receiver
   * - ``syncDeltaMs``
     - double
     - Timestamp delta between cam1 and cam2, ms
   * - ``syncTotalBw``
     - double
     - Combined bandwidth both cameras, GB/s
   * - ``syncDropped``
     - int
     - Total dropped frames across both cameras
   * - ``connected``
     - bool
     - TCP connection to V3000 is active

BackendClient — Invokable Methods
-----------------------------------

Called from QML as ``backend.methodName()``:

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Method
     - Action
   * - ``connectToV3000(host, port)``
     - Connect to V3000 backend. Default: 192.168.100.1:9100
   * - ``disconnect()``
     - Disconnect from V3000
   * - ``triggerBoth()``
     - Send trigger command for both cameras
   * - ``triggerCam1()``
     - Send trigger command for Camera 1 only
   * - ``triggerCam2()``
     - Send trigger command for Camera 2 only
   * - ``startAll()``
     - Start acquisition on both cameras
   * - ``stopAll()``
     - Stop acquisition on both cameras
   * - ``setExposure(int us)``
     - Set exposure time in microseconds
   * - ``setPixelFormat(QString fmt)``
     - Set pixel format e.g. "BayerGR12"
   * - ``setOutputDir(QString dir)``
     - Set frame output directory on V3000

TCP Protocol Reference
-----------------------

All messages are newline-delimited JSON on TCP port 9100.

**V3000 → Android (10 Hz):** ``{ "type": "status", "cam1": {...}, "cam2": {...}, "gnss": {...}, "sync": {...} }``

**V3000 → Android (per frame):** ``{ "type": "frame", "cam": 1, "index": 1284, "geo_tagged": true, "process_ms": 163, "ts": "14:23:11.861" }``

**Android → V3000:** ``{ "type": "trigger"|"start"|"stop"|"set", ... }``

See :doc:`arch_overview` for the complete message format.
