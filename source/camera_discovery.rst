Camera Discovery and Connection
================================

The entry point to the entire Vimba X C++ API is the ``VmbSystem`` singleton.
Every interaction with cameras starts here.

Initializing the SDK
---------------------

.. code-block:: cpp

   VmbSystem& system = VmbSystem::GetInstance();

   VmbError_t err = system.Startup();
   if (err != VmbErrorSuccess)
   {
       std::cerr << "Startup failed: " << err << std::endl;
       return 1;
   }

.. warning::
   ``VmbSystem::Startup()`` and ``VmbSystem::Shutdown()`` **must always be
   called as a matched pair**. Failing to call ``Shutdown()`` leaves transport
   layers loaded and may cause access violations after your application exits.

Discovering Cameras
--------------------

For GigE cameras, Vimba X must actively send discovery packets on the network
before it can enumerate connected devices.

**Option A — Automatic discovery (recommended for development):**

Call ``GetCameras()`` without registering an observer. Vimba X waits for
camera responses before returning:

.. code-block:: cpp

   CameraPtrVector cameras;
   err = system.GetCameras(cameras);

   if (err != VmbErrorSuccess || cameras.empty())
   {
       std::cerr << "No cameras found." << std::endl;
       system.Shutdown();
       return 1;
   }

**Option B — Observer-based discovery (recommended for production):**

Register a ``CameraListObserver`` first. This causes ``GetCameras()`` to
return immediately, with camera events delivered asynchronously:

.. code-block:: cpp

   class CamObserver : public ICameraListObserver
   {
   public:
       void CameraListChanged(CameraPtr pCam, UpdateTriggerType reason) override
       {
           if (reason == UpdateTriggerPluggedIn)
               std::cout << "Camera connected" << std::endl;
           else if (reason == UpdateTriggerPluggedOut)
               std::cout << "Camera disconnected" << std::endl;
       }
   };

   system.RegisterCameraListObserver(
       ICameraListObserverPtr(new CamObserver()));

   // Now GetCameras returns immediately
   system.GetCameras(cameras);

.. note::
   The following functions **must not** be called from within a
   ``CameraListObserver`` callback: ``Startup``, ``Shutdown``,
   ``GetCameras``, ``GetCameraByID``, ``RegisterCameraListObserver``,
   ``UnregisterCameraListObserver``.

Opening a Camera
-----------------

**By IP address (most reliable for SHR 10GigE):**

.. code-block:: cpp

   CameraPtr camera;
   err = system.OpenCameraByID(
       "192.168.0.42",   // Camera IP address
       VmbAccessModeFull,
       camera);

   if (err != VmbErrorSuccess)
   {
       std::cerr << "Could not open camera: " << err << std::endl;
       system.Shutdown();
       return 1;
   }

You can also open by **MAC address** or **serial number** — pass the identifier
string to ``OpenCameraByID``.

**From the discovered camera list:**

.. code-block:: cpp

   CameraPtr camera = cameras.at(0);   // First found camera
   err = camera->Open(VmbAccessModeFull);

Access Modes
^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Mode
     - Description
   * - ``VmbAccessModeFull``
     - Read + write access. Required for feature configuration and image capture.
   * - ``VmbAccessModeRead``
     - Read-only. For monitoring a camera already in use by another application.
       GigE cameras in use by another app can still stream images via Multicast.

Inspecting Camera Information
------------------------------

After opening, read basic camera properties:

.. code-block:: cpp

   std::string name, model, serial;
   camera->GetName(name);
   camera->GetModel(model);
   camera->GetSerialNumber(serial);

   std::cout << "Camera : " << name   << std::endl;
   std::cout << "Model  : " << model  << std::endl;
   std::cout << "Serial : " << serial << std::endl;

Closing the Camera
-------------------

Always close the camera explicitly before shutting down the SDK:

.. code-block:: cpp

   camera->Close();
   system.Shutdown();
