Asynchronous Image Acquisition
================================

The Vimba X SDK distinguishes between two independent operations:

- **Image capture** — the host PC receives frames from the network into buffers
- **Image acquisition** — the camera sensor acquires and transmits frames

You must set up capture (host side) **before** starting acquisition (camera side).

Why Asynchronous?
------------------

Synchronous acquisition handles one buffer at a time — while you process frame N,
the camera is idle. This severely limits throughput, especially for large SHR frames.

Asynchronous acquisition keeps multiple buffers in flight simultaneously. While
your application processes frame N, frame N+1 is already being received into
another buffer:

.. code-block:: text

   Buffer 1: [Filling] → [Processing] → [Requeued] → [Filling again...]
   Buffer 2:             [Filling]    → [Processing] → [Requeued] → ...
   Buffer 3:                           [Filling]    → [Processing] → ...
   Buffer 4:                                          [Filling]    → ...
   Buffer 5:                                                         [Filling] → ...

A minimum of **3 buffers** is recommended. For the SHR at full resolution and
higher frame rates, **5 or more** is advisable.

The Frame Observer
-------------------

Define a class inheriting from ``IFrameObserver``. Vimba X calls
``FrameReceived()`` on a background thread every time a frame completes:

.. code-block:: cpp

   class FrameObserver : public IFrameObserver
   {
   public:
       explicit FrameObserver(CameraPtr pCamera)
           : IFrameObserver(pCamera)
       {}

       void FrameReceived(const FramePtr pFrame) override
       {
           // 1. Always check the frame status first
           VmbFrameStatusType status;
           if (VmbErrorSuccess != pFrame->GetReceiveStatus(status) ||
               status != VmbFrameStatusComplete)
           {
               m_pCamera->QueueFrame(pFrame);  // Requeue and skip
               return;
           }

           // 2. Extract frame data
           VmbUint32_t width = 0, height = 0;
           VmbPixelFormat_t format{};
           void* pData = nullptr;

           pFrame->GetWidth(width);
           pFrame->GetHeight(height);
           pFrame->GetPixelFormat(format);
           pFrame->GetImage(pData);

           // 3. Process the frame (see Image Transformation)
           processFrame(pData, format, width, height);

           // 4. Requeue the buffer for the next frame — do this ASAP
           m_pCamera->QueueFrame(pFrame);
       }
   };

.. warning::
   **Do not perform heavy image processing inside** ``FrameReceived()``.
   The API may block while this callback is executing. Extract the data,
   signal a worker thread, and return. The following functions must not
   be called from within ``FrameReceived()``: ``Startup``, ``Shutdown``,
   ``OpenCameraByID``, ``Open``, ``Close``, ``AcquireSingleImage``,
   ``StartContinuousImageAcquisition``, ``StartCapture``, ``EndCapture``,
   ``AnnounceFrame``, ``RevokeFrame``.

Buffer Allocation and Announcement
------------------------------------

Query the required buffer size, allocate frames, and hand them to the API:

.. code-block:: cpp

   const int BUFFER_COUNT = 5;

   // Get the payload size for one frame
   VmbUint32_t payloadSize = 0;
   camera->GetPayloadSize(payloadSize);

   std::cout << "Frame size: " << payloadSize / 1024 / 1024 << " MB" << std::endl;

   // Create observer and frames
   IFrameObserverPtr observer(new FrameObserver(camera));
   FramePtrVector frames(BUFFER_COUNT);

   for (auto& frame : frames)
   {
       frame.reset(new Frame(payloadSize));
       frame->RegisterObserver(observer);
       camera->AnnounceFrame(frame);
   }

.. tip::
   Use ``VmbPayloadSizeGet()`` (the C API convenience function) or
   ``camera->GetPayloadSize()`` (C++ API). The stream features may carry
   additional payload size information — always use these functions rather
   than computing size from width × height × bytes-per-pixel.

Starting the Capture Engine
-----------------------------

After announcing frames, start the capture engine and queue all buffers:

.. code-block:: cpp

   camera->StartCapture();

   for (auto& frame : frames)
       camera->QueueFrame(frame);

Starting Acquisition on the Camera
------------------------------------

Now start the camera sensor:

.. code-block:: cpp

   FeaturePtr feature;

   camera->GetFeatureByName("AcquisitionMode", feature);
   feature->SetValue("Continuous");

   camera->GetFeatureByName("AcquisitionStart", feature);
   feature->RunCommand();

   std::cout << "Acquisition running..." << std::endl;

Stopping Acquisition — The Correct Sequence
--------------------------------------------

This sequence **must be followed in order**. Skipping steps can cause
crashes or resource leaks:

.. code-block:: cpp

   // 1. Stop the camera sensor
   camera->GetFeatureByName("AcquisitionStop", feature);
   feature->RunCommand();

   // 2. Stop the capture engine (waits for all callbacks to finish)
   camera->EndCapture();

   // 3. Cancel all frames on the queue
   camera->FlushQueue();

   // 4. Release all frame buffers
   camera->RevokeAllFrames();

   // 5. Close the camera
   camera->Close();

   // 6. Shut down the SDK
   system.Shutdown();

Using the Convenience Functions
---------------------------------

For simpler applications, Vimba X provides convenience functions that wrap
the above steps:

.. code-block:: cpp

   // Start continuous acquisition with 5 buffers
   camera->StartContinuousImageAcquisition(
       5, IFrameObserverPtr(new FrameObserver(camera)));

   // ... wait ...

   // Stop and clean up
   camera->StopContinuousImageAcquisition();

.. note::
   Convenience functions perform all steps internally, but they are not optimal
   for applications that frequently start and stop acquisition, since they
   allocate and release buffers each time. For production pipelines, use the
   manual step-by-step approach shown above.
