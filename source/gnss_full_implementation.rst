Full Implementation — File Listing & Build
==========================================

This section summarises all new files added for the GNSS feature,
the updated CMakeLists.txt, and the final build and test instructions.

New Files
----------

.. code-block:: text

   shr_camera_app/
   ├── CMakeLists.txt           ← updated with Qt6::SerialPort
   ├── main.cpp
   ├── MainWindow.h             ← updated with GNSS members and slots
   ├── MainWindow.cpp           ← updated with GNSS panel and slots
   ├── SHR_Capture_App.cpp      ← updated FrameObserver with GNSS snapshot
   ├── FramePayload.h           ← NEW: frame + gnss bundle struct
   ├── GnssRecord.h             ← NEW: shared GNSS data structure
   ├── NmeaReader.h             ← NEW: QSerialPort + NMEA parser
   ├── NmeaReader.cpp           ← NEW: parser implementation
   ├── SidecarWriter.h          ← NEW: JSON sidecar interface
   └── SidecarWriter.cpp        ← NEW: JSON writer implementation

Updated CMakeLists.txt
-----------------------

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.16)
   project(SHR_Camera_App CXX)

   set(CMAKE_CXX_STANDARD 17)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)
   set(CMAKE_AUTOMOC ON)
   set(CMAKE_AUTOUIC ON)
   set(CMAKE_AUTORCC ON)

   # Qt 6 — now includes SerialPort
   find_package(Qt6 REQUIRED COMPONENTS
       Core Widgets Gui SerialPort)

   # Vimba X
   if(NOT DEFINED VIMBAX_DIR)
       set(VIMBAX_DIR "$ENV{HOME}/VimbaX_2026-1" CACHE PATH "")
   endif()
   set(VIMBAX_API_DIR "${VIMBAX_DIR}/api")
   include_directories("${VIMBAX_API_DIR}/include")
   set(VMB_CPP_LIB "${VIMBAX_API_DIR}/lib/x86_64/libVmbCPP.so")
   set(VMB_IMG_LIB "${VIMBAX_API_DIR}/lib/x86_64/libVmbImageTransform.so")

   add_executable(SHR_Camera_App
       main.cpp
       MainWindow.cpp
       SHR_Capture_App.cpp
       NmeaReader.cpp
       SidecarWriter.cpp
   )

   target_link_libraries(SHR_Camera_App
       PRIVATE
           Qt6::Core Qt6::Widgets Qt6::Gui Qt6::SerialPort
           ${VMB_CPP_LIB} ${VMB_IMG_LIB}
           pthread
   )

   add_custom_command(TARGET SHR_Camera_App POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
           ${VMB_CPP_LIB} ${VMB_IMG_LIB}
           $<TARGET_FILE_DIR:SHR_Camera_App>)

Install Qt SerialPort
----------------------

.. code-block:: bash

   sudo apt install qt6-serialport-dev

Build
------

.. code-block:: bash

   cmake -S . -B build -DVIMBAX_DIR=~/VimbaX_2026-1
   cmake --build build -j$(nproc)

Testing Without a Camera — NMEA Simulation
-------------------------------------------

Test the GNSS pipeline without a physical receiver using a virtual serial
port pair (``socat``):

.. code-block:: bash

   # Install socat
   sudo apt install socat

   # Create a virtual serial port pair
   socat PTY,link=/tmp/ttyGNSS0,raw,echo=0 \
         PTY,link=/tmp/ttyGNSS1,raw,echo=0 &

   # Stream sample NMEA sentences into one end
   while true; do
     echo '$GNGGA,142311.000,4808.2292,N,01134.5674,E,1,09,0.9,524.3,M,47.8,M,,*40'
     echo '$GNRMC,142311.000,A,4808.2292,N,01134.5674,E,0.00,0.00,070626,,,A*6F'
     sleep 1
   done > /tmp/ttyGNSS0

   # In the Qt app: select port /tmp/ttyGNSS1, baud 9600, click Connect

.. note::
   The checksums in the example sentences above are illustrative — recalculate
   them for your actual test sentences using the XOR algorithm in
   :doc:`gnss_nmea_primer`. A mismatch will be silently discarded by the parser.

Verifying Sidecar Output
--------------------------

After a test capture, inspect a sidecar file:

.. code-block:: bash

   # Pretty-print the JSON sidecar for frame 1
   cat frames/frame_00001.json | python3 -m json.tool

Expected output structure::

   {
     "frame_index": 1,
     "frame_filename": "frame_00001.ppm",
     "capture_timestamp_utc": "2026-06-07T14:23:11.847Z",
     ...
     "gnss": {
       "valid": true,
       "fix_quality": 1,
       "fix_description": "GPS fix",
       "latitude_deg": 48.137154,
       "longitude_deg": 11.576124,
       ...
     }
   }

Converting Sidecar Data to GeoTIFF
------------------------------------

If you need frames as GeoTIFF with embedded coordinates (e.g. for
photogrammetry software), convert using GDAL after capture:

.. code-block:: bash

   # Install GDAL
   sudo apt install gdal-bin

   # Convert PPM + sidecar → GeoTIFF with embedded coordinates
   # (extract lat/lon from JSON sidecar first)
   LAT=$(python3 -c "import json; d=json.load(open('frames/frame_00001.json')); print(d['gnss']['latitude_deg'])")
   LON=$(python3 -c "import json; d=json.load(open('frames/frame_00001.json')); print(d['gnss']['longitude_deg'])")

   gdal_translate -a_srs EPSG:4326 \
       -a_ullr $LON $LAT $LON $LAT \
       frames/frame_00001.ppm \
       frames/frame_00001_geo.tif

.. tip::
   For full photogrammetric workflows (orthorectification, point cloud
   generation), tools like **OpenDroneMap**, **Agisoft Metashape**, or
   **COLMAP** can ingest the JSON sidecar coordinates directly as camera
   position inputs. The sidecar format used here is easily adaptable to
   the XMP/EXIF geolocation tags these tools expect.

References
-----------

- Qt SerialPort documentation: https://doc.qt.io/qt-6/qserialport.html
- Qt Positioning NMEA plugin: https://doc.qt.io/qt-6/position-plugin-serialnmea.html
- NMEA 0183 standard: https://www.nmea.org/content/STANDARDS/NMEA_0183_Standard
- u-blox F9P RTK receiver: https://www.u-blox.com/en/product/zed-f9p-module
- GDAL: https://gdal.org/
- OpenDroneMap: https://opendronemap.org/
