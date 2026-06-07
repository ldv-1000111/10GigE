Saving Frames to Disk
======================

After transforming a raw Bayer frame into RGB8, saving it to disk in a
portable format allows inspection with standard tools.

PPM Format (Recommended for Development)
-----------------------------------------

PPM (Portable Pixel Map) is a simple uncompressed RGB format. No external
library is required — write it with standard file I/O.

.. code-block:: cpp

   #include <fstream>
   #include <sstream>
   #include <iomanip>

   void saveAsPPM(
       const std::vector<uint8_t>& rgbData,
       VmbUint32_t width,
       VmbUint32_t height,
       int frameIndex,
       const std::string& outputDir)
   {
       std::ostringstream path;
       path << outputDir << "/frame_"
            << std::setw(5) << std::setfill('0') << frameIndex
            << ".ppm";

       std::ofstream file(path.str(), std::ios::binary);
       if (!file)
       {
           std::cerr << "Could not write: " << path.str() << std::endl;
           return;
       }

       // PPM P6 binary header
       file << "P6\n" << width << " " << height << "\n255\n";

       // Raw RGB pixel data (packed, 3 bytes per pixel)
       file.write(reinterpret_cast<const char*>(rgbData.data()),
                  rgbData.size());

       std::cout << "Saved: " << path.str() << std::endl;
   }

.. tip::
   View ``.ppm`` files with:

   - ``display frame_00001.ppm`` (ImageMagick)
   - ``eog frame_00001.ppm`` (GNOME Image Viewer)
   - GIMP (open directly)
   - Convert to PNG: ``convert frame_00001.ppm frame_00001.png``

Output Directory
-----------------

Create the output directory before starting acquisition:

.. code-block:: bash

   mkdir -p ./frames

Or from within the application:

.. code-block:: cpp

   #include <filesystem>

   std::filesystem::create_directories("./frames");

Memory Considerations for SHR
-------------------------------

At full resolution, SHR frames are very large:

.. list-table::
   :header-rows: 1
   :widths: 30 25 25 20

   * - Model
     - Resolution
     - Raw frame (12-bit Bayer)
     - RGB8 output
   * - shr461 (101 MP)
     - 11648 × 8742
     - ~152 MB
     - ~291 MB
   * - shr661 (127 MP)
     - 13392 × 9528
     - ~191 MB
     - ~365 MB
   * - shr411 (151 MP)
     - ~17000 × ~9000
     - ~230 MB
     - ~437 MB
   * - 245 MP top model
     - ~17000 × ~14000
     - ~357 MB
     - ~680 MB

.. warning::
   Saving full-resolution RGB8 frames to disk at any meaningful frame rate
   requires a fast NVMe SSD. A spinning hard disk will become the bottleneck
   within seconds. Consider:

   - Saving every Nth frame
   - Writing in a separate thread (don't block ``FrameReceived()``)
   - Saving the raw Bayer data instead of the RGB8 output, then converting offline

Saving Raw Bayer Data
----------------------

To save the raw 12-bit Bayer frame directly (smaller, faster, lossless):

.. code-block:: cpp

   void saveRawBayer(
       void*        pData,
       VmbUint32_t  payloadSize,
       int          frameIndex,
       const std::string& outputDir)
   {
       std::ostringstream path;
       path << outputDir << "/raw_"
            << std::setw(5) << std::setfill('0') << frameIndex
            << ".bin";

       std::ofstream file(path.str(), std::ios::binary);
       file.write(static_cast<const char*>(pData), payloadSize);
   }

Convert offline later with the Image Transform Library or tools like
``rawpy`` (Python) and ImageMagick.

Writing from a Worker Thread
------------------------------

To keep ``FrameReceived()`` fast, copy the data and signal a worker thread:

.. code-block:: cpp

   class FrameObserver : public IFrameObserver
   {
       std::mutex              m_mutex;
       std::condition_variable m_cv;
       std::vector<uint8_t>    m_pendingFrame;
       bool                    m_frameReady = false;
       std::thread             m_workerThread;

   public:
       FrameObserver(CameraPtr pCamera) : IFrameObserver(pCamera)
       {
           m_workerThread = std::thread([this]{ writerLoop(); });
       }

       void FrameReceived(const FramePtr pFrame) override
       {
           VmbFrameStatusType status;
           if (VmbErrorSuccess != pFrame->GetReceiveStatus(status) ||
               status != VmbFrameStatusComplete)
           {
               m_pCamera->QueueFrame(pFrame);
               return;
           }

           VmbUint32_t w = 0, h = 0;
           VmbPixelFormat_t fmt{};
           void* pData = nullptr;
           pFrame->GetWidth(w);
           pFrame->GetHeight(h);
           pFrame->GetPixelFormat(fmt);
           pFrame->GetImage(pData);

           // Transform and copy — fast path
           std::vector<uint8_t> rgb(w * h * 3);
           transformToRGB8(pData, fmt, w, h, rgb);

           {
               std::lock_guard<std::mutex> lock(m_mutex);
               m_pendingFrame = std::move(rgb);
               m_frameReady   = true;
           }
           m_cv.notify_one();

           // Requeue immediately
           m_pCamera->QueueFrame(pFrame);
       }

       void writerLoop()
       {
           int index = 0;
           while (true)
           {
               std::unique_lock<std::mutex> lock(m_mutex);
               m_cv.wait(lock, [this]{ return m_frameReady; });
               auto frame    = std::move(m_pendingFrame);
               m_frameReady  = false;
               lock.unlock();
               if (frame.empty()) break;   // Shutdown signal
               saveAsPPM(frame, /* width, height */ 0, 0, index++, "./frames");
           }
       }
   };
