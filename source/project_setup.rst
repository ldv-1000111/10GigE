Project Setup
=============

This page covers the Qt installation required on the **laptop** (dev
machine) and the project structure of ``shr_backend``. The NUC receives
only compiled binaries — no Qt development tools are needed on it.

----

Qt Installation — Laptop
--------------------------

The laptop needs the full Qt 6 development stack to build the backend
and, later, the Android APK.

**Option A — apt (Ubuntu 24.04, recommended for backend only):**

.. code-block:: bash

   sudo apt install \
       qt6-base-dev \
       qt6-base-dev-tools \
       qt6-serialport-dev \
       libqt6serialport6-dev

   # Verify
   qmake6 --version
   # Expected: QMake version 3.1, Qt version 6.4.x or higher

.. note::
   Ubuntu 24.04 ships Qt 6.4.x via ``apt``. This is sufficient for
   building the backend. For Qt 6.8 LTS (required for the Android
   frontend), use Option B.

**Option B — Qt Online Installer (required for Android build):**

Download from: https://www.qt.io/download-qt-installer

Select the following components:

- **Qt 6.8 LTS → Desktop gcc 64-bit** — for the backend build
- **Qt 6.8 LTS → Android ARM64-v8a** — for the Android APK
- **Qt Creator 13.x** — IDE with Android wizard

After installing, add to ``~/.bashrc``:

.. code-block:: bash

   export PATH=~/Qt/6.8.0/gcc_64/bin:$PATH
   export Qt6_DIR=~/Qt/6.8.0/gcc_64/lib/cmake/Qt6

   source ~/.bashrc
   qmake6 --version
   # Expected: Qt version 6.8.x

Qt Installation — NUC
-----------------------

The NUC runs the compiled binary and needs only the Qt6 **runtime**
libraries — no development headers, no ``qmake``, no Qt Creator:

.. code-block:: bash

   sudo apt-get install -y \
       libqt6core6t64 \
       libqt6network6t64 \
       libqt6serialport6

These are installed as part of the NUC initial setup in
:doc:`nuc_setup` Step 2. No further Qt configuration is needed on
the NUC.

----

Repository Structure
---------------------

The project lives in a single git repository with two top-level
directories — one per deployable binary:

.. code-block:: text

   shr-10gige-bedrock-docs/          ← git repo root
   │
   ├── source/                       ← Sphinx documentation (RST)
   │
   ├── shr_backend/                  ← V3000 / NUC headless daemon
   │   ├── CMakeLists.txt
   │   ├── README.md
   │   ├── .gitignore
   │   ├── src/
   │   │   └── main.cpp
   │   ├── camera/
   │   │   ├── CameraWorker.h
   │   │   └── CameraWorker.cpp
   │   ├── gnss/
   │   │   ├── GnssRecord.h
   │   │   ├── NmeaReader.h
   │   │   └── NmeaReader.cpp
   │   ├── transform/
   │   │   ├── ImageTransform.h
   │   │   └── ImageTransform.cpp
   │   ├── storage/
   │   │   ├── SidecarWriter.h
   │   │   └── SidecarWriter.cpp
   │   ├── trigger/
   │   │   ├── TriggerServer.h
   │   │   └── TriggerServer.cpp
   │   ├── server/
   │   │   ├── BackendServer.h
   │   │   └── BackendServer.cpp
   │   └── systemd/
   │       └── shr-backend.service
   │
   └── shr_android/                  ← Android Qt QML frontend (Stage 2)
       ├── CMakeLists.txt
       ├── main.cpp
       ├── BackendClient.h/.cpp
       └── qml/

----

shr_backend/CMakeLists.txt
---------------------------

The backend has its own standalone ``CMakeLists.txt`` — it is built
independently from the ``shr_backend/`` directory:

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.16)
   project(SHR_Backend CXX)

   set(CMAKE_CXX_STANDARD 17)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)
   set(CMAKE_AUTOMOC ON)

   # Qt 6 — headless backend only needs Core, Network, SerialPort
   find_package(Qt6 6.2 REQUIRED COMPONENTS Core Network SerialPort)

   # Vimba X SDK
   if(NOT DEFINED VIMBAX_DIR)
       set(VIMBAX_DIR "$ENV{VIMBAX_DIR}" CACHE PATH "Vimba X install dir")
   endif()
   if(NOT VIMBAX_DIR OR NOT EXISTS "${VIMBAX_DIR}")
       set(VIMBAX_DIR "$ENV{HOME}/VimbaX_2026-1" CACHE PATH "" FORCE)
   endif()

   set(VIMBAX_API "${VIMBAX_DIR}/api")
   include_directories(
       "${VIMBAX_API}/include"
       "${CMAKE_CURRENT_SOURCE_DIR}"
   )

   set(VMB_CPP "${VIMBAX_API}/lib/libVmbCPP.so")
   set(VMB_IMG "${VIMBAX_API}/lib/libVmbImageTransform.so")

   add_executable(SHR_Backend
       src/main.cpp
       camera/CameraWorker.cpp
       gnss/NmeaReader.cpp
       transform/ImageTransform.cpp
       storage/SidecarWriter.cpp
       trigger/TriggerServer.cpp
       server/BackendServer.cpp
   )

   target_link_libraries(SHR_Backend
       PRIVATE
           Qt6::Core Qt6::Network Qt6::SerialPort
           ${VMB_CPP} ${VMB_IMG}
           pthread
   )

   # Copy Vimba X libs next to binary at build time
   add_custom_command(TARGET SHR_Backend POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
           ${VMB_CPP} ${VMB_IMG}
           $<TARGET_FILE_DIR:SHR_Backend>
   )

   # Copy Camera Simulator TL if present
   set(SIM_CTI "${VIMBAX_DIR}/cti/VimbaCameraSimulatorTL.cti")
   if(EXISTS "${SIM_CTI}")
       add_custom_command(TARGET SHR_Backend POST_BUILD
           COMMAND ${CMAKE_COMMAND} -E copy_if_different
               "${SIM_CTI}" $<TARGET_FILE_DIR:SHR_Backend>
       )
   endif()

----

Building
---------

All build commands run from the ``shr_backend/`` directory:

.. code-block:: bash

   cd shr_backend

   # Configure (first time only, or after CMakeLists changes)
   cmake -S . -B build \
       -DCMAKE_BUILD_TYPE=Release \
       -DVIMBAX_DIR=${VIMBAX_DIR}

   # Build
   cmake --build build -j$(nproc)

   # Verify output
   ls -lh build/SHR_Backend
   ls -lh build/libVmbCPP.so
   ls -lh build/libVmbImageTransform.so

For a debug build (useful during development):

.. code-block:: bash

   cmake -S . -B build-debug \
       -DCMAKE_BUILD_TYPE=Debug \
       -DVIMBAX_DIR=${VIMBAX_DIR}
   cmake --build build-debug -j$(nproc)
