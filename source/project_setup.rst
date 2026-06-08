Project Setup
=============

This section sets up the CMake build system for the Qt capture application
on the Bedrock V3000 running Linux.

Install Qt
-----------

Qt 6 can be installed via the system package manager (recommended for Linux)
or via the `Qt online installer <https://www.qt.io/download-qt-installer>`_.
Full Qt 6 documentation is at https://doc.qt.io/qt-6/

.. code-block:: bash

   # Ubuntu 24.04 LTS
   sudo apt install qt6-base-dev qt6-base-dev-tools cmake ninja-build

   # Verify — should report 6.8.x or higher
   qmake6 --version

**Qt 6.8 LTS** is the recommended version for this project — it is the
current long-term support release, supported until October 2029, and
the correct choice for industrial applications requiring multi-year stability.

Install via the `Qt online installer <https://www.qt.io/download-qt-installer>`_
(select Qt 6.8 LTS) or via the system package manager if 6.8 is available:

Directory Structure
--------------------

.. code-block:: text

   shr_camera_app/
   ├── CMakeLists.txt
   ├── main.cpp
   ├── MainWindow.h
   ├── MainWindow.cpp
   └── SHR_Capture_App.cpp      ← Vimba X acquisition (non-Qt)

CMakeLists.txt
--------------

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.16)
   project(SHR_Camera_App CXX)

   set(CMAKE_CXX_STANDARD 17)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)
   set(CMAKE_AUTOMOC ON)
   set(CMAKE_AUTOUIC ON)
   set(CMAKE_AUTORCC ON)

   # Qt 6
   find_package(Qt6 6.8 REQUIRED COMPONENTS Core Widgets Gui)

   # Vimba X — adjust path as needed
   if(NOT DEFINED VIMBAX_DIR)
       set(VIMBAX_DIR "$ENV{HOME}/VimbaX_2026-1" CACHE PATH "Vimba X install dir")
   endif()
   set(VIMBAX_API_DIR "${VIMBAX_DIR}/api")

   include_directories("${VIMBAX_API_DIR}/include")

   set(VMB_CPP_LIB "${VIMBAX_API_DIR}/lib/x86_64/libVmbCPP.so")
   set(VMB_IMG_LIB "${VIMBAX_API_DIR}/lib/x86_64/libVmbImageTransform.so")

   add_executable(SHR_Camera_App
       main.cpp
       MainWindow.cpp
       SHR_Capture_App.cpp
   )

   target_link_libraries(SHR_Camera_App
       PRIVATE
           Qt6::Core Qt6::Widgets Qt6::Gui
           ${VMB_CPP_LIB} ${VMB_IMG_LIB}
           pthread
   )

   # Copy Vimba X libs next to binary
   add_custom_command(TARGET SHR_Camera_App POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
           ${VMB_CPP_LIB} ${VMB_IMG_LIB}
           $<TARGET_FILE_DIR:SHR_Camera_App>)

Key Thread Safety Rules
------------------------

Because Vimba X callbacks run on a background thread and Qt UI updates
must happen on the main thread, all communication between the two must
use Qt's thread-safe mechanisms:

.. code-block:: cpp

   // CORRECT — signal/slot across threads (Qt queues the call)
   emit frameReceived(frameIndex, thumbnail);

   // CORRECT — invoke on main thread from any thread
   QMetaObject::invokeMethod(this, "updateStatus",
       Qt::QueuedConnection,
       Q_ARG(QString, statusMsg));

   // WRONG — direct UI update from Vimba X callback thread
   m_label->setText("Frame received");  // crash or corruption

Verify the Build
-----------------

.. code-block:: bash

   cmake -S . -B build -DVIMBAX_DIR=~/VimbaX_2026-1
   cmake --build build -j$(nproc)
   ./build/SHR_Camera_App
