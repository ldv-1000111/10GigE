Building and Running
=====================

Prerequisites Check
--------------------

Before building, verify:

.. code-block:: bash

   # Compiler (need C++17 support)
   g++ --version        # GCC 9+ required
   cmake --version      # 3.16+ required

   # Vimba X installed
   ls ~/VimbaX_2026-1/api/include/VmbCPP/VmbCPP.h

   # GigE Transport Layer installed
   echo $GENICAM_GENTL64_PATH

   # Runtime libs accessible
   echo $LD_LIBRARY_PATH | grep VimbaX

Build with CMake
-----------------

.. code-block:: bash

   # From your project directory (contains CMakeLists.txt and SHR_Capture_App.cpp)
   cmake -S . -B build -DVIMBAX_DIR=~/VimbaX_2026-1

   cmake --build build

   # Binary is at:
   ls build/SHR_Capture_App

If Vimba X is installed in a non-default location, pass the path explicitly:

.. code-block:: bash

   cmake -S . -B build -DVIMBAX_DIR=/opt/VimbaX_2026-1

Running the Application
------------------------

**First found camera, 10 frames:**

.. code-block:: bash

   ./build/SHR_Capture_App

**Specific camera by IP, 25 frames:**

.. code-block:: bash

   ./build/SHR_Capture_App 192.168.0.42 25

**Continuous capture until Ctrl+C:**

.. code-block:: bash

   ./build/SHR_Capture_App 192.168.0.42 0

Expected output::

   [SHR] === SHR 10GigE Capture & Transform App ===
   [SHR] Camera IP  : 192.168.0.42
   [SHR] Frames     : 25
   [SHR] Buffers    : 5
   [SHR] Format     : BayerGR12
   [SHR] Output     : ./frames
   [SHR] Vimba X started
   [SHR] Opened camera: SHR 411xXGE | Model: shr411xXGE | S/N: 12345678
   [SHR] Resolution set to: 17000 x 9000
   [SHR] Payload size: 229 MB per frame
   [SHR] Acquisition started — press Ctrl+C to stop
   [SHR] Frame 1: 17000x9000
   [SHR] Saved: ./frames/frame_00001.ppm
   [SHR] Frame 2: 17000x9000
   [SHR] Saved: ./frames/frame_00002.ppm
   ...
   [SHR] Done. Captured 25 frame(s).

Viewing Saved Frames
---------------------

.. code-block:: bash

   # View with ImageMagick
   display ./frames/frame_00001.ppm

   # Convert to PNG
   convert ./frames/frame_00001.ppm ./frames/frame_00001.png

   # View with GNOME image viewer
   eog ./frames/

   # Check image dimensions
   identify ./frames/frame_00001.ppm

Modifying the Configuration
-----------------------------

Edit the ``AppConfig`` struct in ``SHR_Capture_App.cpp`` to change defaults:

.. code-block:: cpp

   struct AppConfig
   {
       std::string cameraIP     = "192.168.0.42"; // Fixed IP
       int         frameCount   = 0;              // Continuous
       int         bufferCount  = 8;              // More buffers for high res
       std::string pixelFormat  = "BayerGR12";
       bool        saveFrames   = true;
       std::string outputDir    = "/mnt/fast-ssd/frames"; // Fast storage
       bool        colorCorrect = true;
       int         debayerMode  = 1;              // 3x3 better quality
   };
