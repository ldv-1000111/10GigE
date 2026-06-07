Full Application Source
========================

Below is the complete, annotated source code for the SHR 10GigE capture and
image transformation application. Every section corresponds to a chapter in
this tutorial.

.. tip::
   Download the source file directly: :download:`SHR_Capture_App.cpp <_static/SHR_Capture_App.cpp>`

SHR_Capture_App.cpp
--------------------

Includes and configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   #include <iostream>
   #include <fstream>
   #include <sstream>
   #include <string>
   #include <vector>
   #include <atomic>
   #include <thread>
   #include <chrono>
   #include <mutex>
   #include <condition_variable>
   #include <csignal>
   #include <iomanip>

   #include "VmbCPP/VmbCPP.h"
   #include "VmbImageTransform/VmbTransform.h"

   using namespace VmbCPP;

   struct AppConfig
   {
       std::string cameraIP     = "";          // Empty = first found
       int         frameCount   = 10;          // 0 = continuous
       int         bufferCount  = 5;
       std::string pixelFormat  = "BayerGR12";
       bool        saveFrames   = true;
       std::string outputDir    = "./frames";
       bool        colorCorrect = false;
       int         debayerMode  = 0;           // 0=2x2, 1=3x3, 2=LCAA, 3=LCAAV
   };

   static std::atomic<bool> g_running{true};
   static std::atomic<int>  g_framesCaptured{0};
   static AppConfig         g_config;

   void signalHandler(int) { g_running = false; }

Image transformation function
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   bool transformFrame(
       void*              pSrcData,
       VmbPixelFormat_t   srcFormat,
       VmbUint32_t        width,
       VmbUint32_t        height,
       std::vector<uint8_t>& outBuffer)
   {
       VmbImage sourceImage{};
       VmbImage destinationImage{};
       sourceImage.Size      = sizeof(sourceImage);
       destinationImage.Size = sizeof(destinationImage);
       sourceImage.Data      = pSrcData;
       destinationImage.Data = outBuffer.data();

       VmbError_t err;
       err = VmbSetImageInfoFromPixelFormat(srcFormat, width, height, &sourceImage);
       if (err != VmbErrorSuccess) return false;

       err = VmbSetImageInfoFromInputImage(&sourceImage, VmbPixelLayoutRGB, 8,
                                           &destinationImage);
       if (err != VmbErrorSuccess) return false;

       std::vector<VmbTransformInfo> params;

       VmbTransformInfo debayerInfo{};
       VmbDebayerMode_t mode = VmbDebayerMode2x2;
       if (g_config.debayerMode == 1) mode = VmbDebayerMode3x3;
       if (g_config.debayerMode == 2) mode = VmbDebayerModeLCAA;
       if (g_config.debayerMode == 3) mode = VmbDebayerModeLCAAV;
       VmbSetDebayerMode(mode, &debayerInfo);
       params.push_back(debayerInfo);

       if (g_config.colorCorrect)
       {
           VmbTransformInfo ccInfo{};
           const VmbFloat_t mat[] = { 1.0f,0.0f,0.0f, 0.0f,1.0f,0.0f, 0.0f,0.0f,1.0f };
           VmbSetColorCorrectionMatrix3x3(mat, &ccInfo);
           params.push_back(ccInfo);
       }

       err = VmbImageTransform(&sourceImage, &destinationImage,
                               params.data(),
                               static_cast<VmbUint32_t>(params.size()));
       return err == VmbErrorSuccess;
   }

Save frame as PPM
^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   void saveFrame(const std::vector<uint8_t>& rgb,
                  VmbUint32_t w, VmbUint32_t h, int index)
   {
       std::ostringstream path;
       path << g_config.outputDir << "/frame_"
            << std::setw(5) << std::setfill('0') << index << ".ppm";

       std::ofstream file(path.str(), std::ios::binary);
       if (!file) { std::cerr << "Cannot write: " << path.str() << std::endl; return; }

       file << "P6\n" << w << " " << h << "\n255\n";
       file.write(reinterpret_cast<const char*>(rgb.data()), rgb.size());
       std::cout << "[SHR] Saved: " << path.str() << std::endl;
   }

Frame observer
^^^^^^^^^^^^^^^

.. code-block:: cpp

   class FrameObserver : public IFrameObserver
   {
   public:
       explicit FrameObserver(CameraPtr pCamera) : IFrameObserver(pCamera) {}

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

           int idx = ++g_framesCaptured;
           std::cout << "[SHR] Frame " << idx
                     << ": " << w << "x" << h << std::endl;

           std::vector<uint8_t> rgb(w * h * 3);
           if (transformFrame(pData, fmt, w, h, rgb) && g_config.saveFrames)
               saveFrame(rgb, w, h, idx);

           if (g_config.frameCount > 0 && idx >= g_config.frameCount)
           {
               g_running = false;
               return;   // Don't requeue — we're done
           }

           m_pCamera->QueueFrame(pFrame);
       }
   };

Feature helpers
^^^^^^^^^^^^^^^^

.. code-block:: cpp

   VmbError_t setEnum(CameraPtr& cam, const char* name, const char* val)
   {
       FeaturePtr f; VmbError_t e = cam->GetFeatureByName(name, f);
       if (e != VmbErrorSuccess) return e;
       return f->SetValue(val);
   }

   VmbError_t setInt(CameraPtr& cam, const char* name, VmbInt64_t val)
   {
       FeaturePtr f; VmbError_t e = cam->GetFeatureByName(name, f);
       if (e != VmbErrorSuccess) return e;
       return f->SetValue(val);
   }

   VmbError_t runCmd(CameraPtr& cam, const char* name)
   {
       FeaturePtr f; VmbError_t e = cam->GetFeatureByName(name, f);
       if (e != VmbErrorSuccess) return e;
       return f->RunCommand();
   }

   VmbError_t getInt(CameraPtr& cam, const char* name, VmbInt64_t& val)
   {
       FeaturePtr f; VmbError_t e = cam->GetFeatureByName(name, f);
       if (e != VmbErrorSuccess) return e;
       return f->GetValue(val);
   }

main() — startup and camera open
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   int main(int argc, char* argv[])
   {
       if (argc > 1) g_config.cameraIP   = argv[1];
       if (argc > 2) g_config.frameCount = std::stoi(argv[2]);

       std::signal(SIGINT,  signalHandler);
       std::signal(SIGTERM, signalHandler);
       std::filesystem::create_directories(g_config.outputDir);

       VmbSystem& system = VmbSystem::GetInstance();
       if (system.Startup() != VmbErrorSuccess) return 1;

       CameraPtr camera;
       if (!g_config.cameraIP.empty())
       {
           system.OpenCameraByID(g_config.cameraIP.c_str(),
                                 VmbAccessModeFull, camera);
       }
       else
       {
           CameraPtrVector cameras;
           system.GetCameras(cameras);
           camera = cameras.at(0);
           camera->Open(VmbAccessModeFull);
       }

main() — configure, acquire, shutdown
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

       setEnum(camera, "PixelFormat",    g_config.pixelFormat.c_str());
       setEnum(camera, "AcquisitionMode","Continuous");
       setInt (camera, "GVSPBurstSize",  64);

       VmbInt64_t maxW = 0, maxH = 0;
       getInt(camera, "WidthMax",  maxW);
       getInt(camera, "HeightMax", maxH);
       if (maxW) setInt(camera, "Width",  maxW);
       if (maxH) setInt(camera, "Height", maxH);

       VmbUint32_t payloadSize = 0;
       camera->GetPayloadSize(payloadSize);

       IFrameObserverPtr observer(new FrameObserver(camera));
       FramePtrVector frames(g_config.bufferCount);
       for (auto& f : frames)
       {
           f.reset(new Frame(payloadSize));
           f->RegisterObserver(observer);
           camera->AnnounceFrame(f);
       }

       camera->StartCapture();
       for (auto& f : frames) camera->QueueFrame(f);
       runCmd(camera, "AcquisitionStart");

       while (g_running)
           std::this_thread::sleep_for(std::chrono::milliseconds(100));

       // Shutdown sequence
       runCmd(camera, "AcquisitionStop");
       camera->EndCapture();
       camera->FlushQueue();
       camera->RevokeAllFrames();
       camera->Close();
       system.Shutdown();

       std::cout << "Captured " << g_framesCaptured.load() << " frame(s)." << std::endl;
       return 0;
   }
