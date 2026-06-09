Project Setup
=============

The project uses a single ``CMakeLists.txt`` that builds two independent
targets depending on the platform: the **V3000 backend** (Linux x86_64,
``QCoreApplication``) and the **Android frontend** (ARM64, ``QGuiApplication``).

Install Qt 6.8 LTS
--------------------

`Qt 6.8 LTS <https://www.qt.io/download-qt-installer>`_ is the recommended
version — supported until October 2029, correct for industrial software.

Install via the Qt Maintenance Tool. Select:

- **Qt 6.8 LTS → Desktop gcc 64-bit** (for V3000 build)
- **Qt 6.8 LTS → Android ARM64-v8a** (for Android build)
- **Qt Creator 13.x** (includes Android wizard)

Or via ``apt`` for the V3000 desktop target only:

.. code-block:: bash

   # Ubuntu 24.04 — V3000 backend dependencies only
   sudo apt install qt6-base-dev qt6-base-dev-tools \
                    cmake ninja-build gcc g++

   # Verify — should report 6.8.x or higher
   qmake6 --version

Directory Structure
--------------------

.. code-block:: text

   shr_camera_project/
   ├── CMakeLists.txt
   │
   ├── shr_backend/              ← V3000 headless daemon (C++)
   │   ├── main_v3000.cpp
   │   ├── camera/
   │   ├── gnss/
   │   ├── transform/
   │   ├── storage/
   │   ├── trigger/
   │   ├── server/
   │   └── systemd/
   │
   └── shr_android/              ← Android Qt QML app
       ├── main_android.cpp
       ├── BackendClient.h/.cpp
       ├── qml/
       ├── styles/
       └── package/

CMakeLists.txt
---------------

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.16)
   project(SHR_Camera_System CXX)

   set(CMAKE_CXX_STANDARD 17)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)

   if(ANDROID)

     # ── Android frontend (Qt QML) ─────────────────────────────────
     find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Quick Network)

     set(CMAKE_AUTOMOC ON)

     qt_add_executable(SHR_Camera_Android
         shr_android/main_android.cpp
         shr_android/BackendClient.cpp
     )

     qt_add_qml_module(SHR_Camera_Android
         URI SHRCamera
         VERSION 1.0
         QML_FILES
             shr_android/qml/MainView.qml
             shr_android/qml/CameraPanel.qml
             shr_android/qml/TriggerBar.qml
             shr_android/qml/RightPanel.qml
             shr_android/qml/GnssPanel.qml
             shr_android/qml/SyncPanel.qml
             shr_android/qml/SectionWidget.qml
             shr_android/qml/FrameLog.qml
         RESOURCES
             shr_android/qml/styles/colors.js
     )

     target_link_libraries(SHR_Camera_Android
         PRIVATE Qt6::Core Qt6::Gui Qt6::Quick Qt6::Network)

     set_target_properties(SHR_Camera_Android PROPERTIES
         QT_ANDROID_PACKAGE_SOURCE_DIR
             "${CMAKE_CURRENT_SOURCE_DIR}/shr_android/package"
         QT_ANDROID_TARGET_SDK_VERSION 34
         QT_ANDROID_MIN_SDK_VERSION    26
         QT_ANDROID_ABIS               "arm64-v8a"
     )

   else()

     # ── V3000 backend (headless daemon) ───────────────────────────
     find_package(Qt6 6.8 REQUIRED COMPONENTS Core Network SerialPort)

     set(CMAKE_AUTOMOC ON)

     # Vimba X SDK
     if(NOT DEFINED VIMBAX_DIR)
         set(VIMBAX_DIR "$ENV{HOME}/VimbaX_2026-1"
             CACHE PATH "Vimba X install dir")
     endif()
     set(VIMBAX_API_DIR "${VIMBAX_DIR}/api")
     include_directories("${VIMBAX_API_DIR}/include")
     set(VMB_CPP_LIB "${VIMBAX_API_DIR}/lib/x86_64/libVmbCPP.so")
     set(VMB_IMG_LIB "${VIMBAX_API_DIR}/lib/x86_64/libVmbImageTransform.so")

     add_executable(SHR_Backend
         shr_backend/main_v3000.cpp
         shr_backend/camera/CameraWorker.cpp
         shr_backend/gnss/NmeaReader.cpp
         shr_backend/transform/ImageTransform.cpp
         shr_backend/storage/SidecarWriter.cpp
         shr_backend/trigger/TriggerServer.cpp
         shr_backend/server/BackendServer.cpp
     )

     target_link_libraries(SHR_Backend
         PRIVATE
             Qt6::Core Qt6::Network Qt6::SerialPort
             ${VMB_CPP_LIB} ${VMB_IMG_LIB}
             pthread
     )

     # Copy Vimba X libs next to the binary
     add_custom_command(TARGET SHR_Backend POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy_if_different
             ${VMB_CPP_LIB} ${VMB_IMG_LIB}
             $<TARGET_FILE_DIR:SHR_Backend>)

   endif()

Building
---------

**V3000 backend (on V3000 or dev machine):**

.. code-block:: bash

   cmake -S . -B build-v3000 \
     -DCMAKE_BUILD_TYPE=Release \
     -DVIMBAX_DIR=~/VimbaX_2026-1
   cmake --build build-v3000 -j$(nproc)

**Android APK (on dev machine):**

.. code-block:: bash

   cmake -S . -B build-android \
     -DCMAKE_SYSTEM_NAME=Android \
     -DCMAKE_ANDROID_ARCH_ABI=arm64-v8a \
     -DCMAKE_ANDROID_NDK=~/Android/Sdk/ndk/26.3.11579264 \
     -DANDROID_PLATFORM=android-26 \
     -DCMAKE_PREFIX_PATH=~/Qt/6.8.0/android_arm64_v8a
   cmake --build build-android --target SHR_Camera_Android
