Frame Tagging in the Acquisition Pipeline
==========================================

GNSS data is injected into the frame at the moment ``FrameReceived()`` fires —
the closest point in time to the actual sensor exposure. The key is that this
must not slow down the callback, so the GNSS snapshot is taken in one atomic
operation and the actual file writing happens on a worker thread.

Updated FrameObserver
----------------------

The frame observer now captures both the frame payload and the GNSS record,
packages them together, and hands them to the worker thread:

.. code-block:: cpp

   // FramePayload.h — data passed from observer to worker thread
   struct FramePayload
   {
       std::vector<uint8_t>          rawData;       // copy of frame bytes
       VmbPixelFormat_t              pixelFormat;
       VmbUint32_t                   width;
       VmbUint32_t                   height;
       VmbUint32_t                   payloadSize;
       int                           frameIndex;
       QDateTime                     captureTime;   // host wall clock
       std::shared_ptr<GnssRecord>   gnss;          // atomic snapshot
   };

.. code-block:: cpp

   // Updated FrameObserver::FrameReceived()
   void FrameObserver::FrameReceived(const FramePtr pFrame)
   {
       VmbFrameStatusType status;
       if (VmbErrorSuccess != pFrame->GetReceiveStatus(status) ||
           status != VmbFrameStatusComplete)
       {
           m_pCamera->QueueFrame(pFrame);
           return;
       }

       // --- Hot path: grab everything atomically, then requeue ---

       VmbUint32_t width = 0, height = 0, payloadSize = 0;
       VmbPixelFormat_t fmt{};
       void* pData = nullptr;

       pFrame->GetWidth(width);
       pFrame->GetHeight(height);
       pFrame->GetPayloadSize(payloadSize);
       pFrame->GetPixelFormat(fmt);
       pFrame->GetImage(pData);

       int idx = ++g_framesCaptured;

       // Snapshot GNSS — single atomic load, zero contention
       auto gnssSnapshot = std::atomic_load(&g_currentGnss);

       // Snapshot host timestamp
       QDateTime captureTime = QDateTime::currentDateTimeUtc();

       // Copy raw frame bytes — necessary before requeueing
       FramePayload payload;
       payload.rawData.assign(
           static_cast<uint8_t*>(pData),
           static_cast<uint8_t*>(pData) + payloadSize);
       payload.pixelFormat  = fmt;
       payload.width        = width;
       payload.height       = height;
       payload.payloadSize  = payloadSize;
       payload.frameIndex   = idx;
       payload.captureTime  = captureTime;
       payload.gnss         = gnssSnapshot;

       // Hand off to worker — does not block
       {
           std::lock_guard<std::mutex> lock(m_queueMutex);
           m_frameQueue.push(std::move(payload));
       }
       m_queueCv.notify_one();

       // Requeue immediately — critical for sustained streaming
       m_pCamera->QueueFrame(pFrame);

       // Stop if frame limit reached
       if (g_config.frameCount > 0 && idx >= g_config.frameCount)
           g_running = false;
   }

Camera Chunk Timestamp (Optional Enhancement)
----------------------------------------------

If chunk data is enabled on the SHR camera, the camera's own hardware
timestamp can be read before requeueing — this is a more precise per-frame
time than the host wall clock:

.. code-block:: cpp

   // Enable chunk mode before acquisition:
   setEnum(camera, "ChunkModeActive", "true");
   setEnum(camera, "ChunkSelector",   "Timestamp");
   setEnum(camera, "ChunkEnable",     "true");

   // In FrameReceived(), read the camera timestamp:
   FeatureContainerPtr chunkData;
   VmbUint64_t cameraTimestampNs = 0;

   if (VmbErrorSuccess == pFrame->AccessChunkData(chunkData))
   {
       FeaturePtr tsFeature;
       if (VmbErrorSuccess == chunkData->GetFeatureByName(
               "ChunkTimestamp", tsFeature))
       {
           VmbInt64_t ts = 0;
           tsFeature->GetValue(ts);
           cameraTimestampNs = static_cast<VmbUint64_t>(ts);
       }
   }

   payload.cameraTimestampNs = cameraTimestampNs;

.. note::
   ``ChunkTimestamp`` availability depends on SHR firmware. Check with
   Vimba X Viewer → camera features → ChunkDataControl to see which
   chunk features your specific camera supports. The host timestamp fallback
   in the base implementation works without chunk data enabled.
