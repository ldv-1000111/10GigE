Qt for Android — Build Environment
=====================================

Setting up the Qt for Android toolchain requires the Android SDK, NDK,
and Qt's Android toolchain. All of this is managed through the Qt
Maintenance Tool and Qt Creator.

Prerequisites
--------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Tool
     - Requirement
   * - Qt 6.8 LTS
     - Android component included (``qt6-android``)
   * - Android NDK
     - r26 or higher (LTS)
   * - Android SDK
     - API level 34 (Android 14) target; min API 26 (Android 8)
   * - JDK
     - OpenJDK 17 (required by Android Gradle)
   * - Qt Creator
     - 13.x or higher (bundled with Qt 6.8)
   * - ADB
     - Installed as part of Android SDK platform tools

Install via Qt Maintenance Tool
---------------------------------

.. code-block:: bash

   # Run the Qt Maintenance Tool
   ~/Qt/MaintenanceTool

   # Select:
   # Qt 6.8 LTS → Android → Qt for Android (ARM64-v8a)
   # Qt Creator (latest)
   # Android SDK (via Qt Creator setup wizard)

Qt Creator will guide you through SDK and NDK installation when you
first configure an Android device. Go to:

.. code-block:: text

   Qt Creator → Preferences → Devices → Android
   → Set Android SDK path: ~/Android/Sdk
   → Set Android NDK path: ~/Android/Sdk/ndk/26.x.x
   → Set JDK path: /usr/lib/jvm/java-17-openjdk-amd64

Qt Creator will download any missing components automatically.

CMakeLists.txt — Android additions
-------------------------------------

The Android app is a separate CMake target in the same repository,
built conditionally when ``CMAKE_SYSTEM_NAME`` is ``Android``:

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.16)
   project(SHR_Camera CXX)

   set(CMAKE_CXX_STANDARD 17)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)

   if(ANDROID)
       # ── Android frontend ──────────────────────────────────────
       find_package(Qt6 6.8 REQUIRED COMPONENTS
           Core Quick Network)

       set(CMAKE_AUTOMOC ON)

       qt_add_executable(SHR_Camera_Android
           android/main_android.cpp
           android/BackendClient.cpp
           android/BackendClient.h
       )

       qt_add_qml_module(SHR_Camera_Android
           URI SHRCamera
           VERSION 1.0
           QML_FILES
               android/qml/MainView.qml
               android/qml/CameraPanel.qml
               android/qml/TriggerBar.qml
               android/qml/RightPanel.qml
               android/qml/GnssPanel.qml
               android/qml/SyncPanel.qml
               android/qml/SectionWidget.qml
               android/qml/FrameLog.qml
           RESOURCES
               android/styles/colors.js
       )

       target_link_libraries(SHR_Camera_Android
           PRIVATE Qt6::Core Qt6::Quick Qt6::Network)

       set_target_properties(SHR_Camera_Android PROPERTIES
           QT_ANDROID_PACKAGE_SOURCE_DIR
               "${CMAKE_CURRENT_SOURCE_DIR}/android/package"
           QT_ANDROID_TARGET_SDK_VERSION 34
           QT_ANDROID_MIN_SDK_VERSION    26
           QT_ANDROID_ABIS               "arm64-v8a"
       )

   else()
       # ── V3000 backend ─────────────────────────────────────────
       find_package(Qt6 6.8 REQUIRED COMPONENTS Core Network)

       # ... existing V3000 build targets
   endif()

Android package structure
--------------------------

The ``android/package/`` directory contains Android-specific files:

.. code-block:: text

   android/
   ├── main_android.cpp           ← Android entry point
   ├── BackendClient.h/.cpp       ← TCP client connecting to V3000
   ├── qml/
   │   ├── MainView.qml           ← root application window
   │   ├── CameraPanel.qml        ← per-camera preview + metrics
   │   ├── SectionWidget.qml      ← collapsible section
   │   ├── TriggerBar.qml         ← trigger + start/stop buttons
   │   ├── RightPanel.qml         ← GNSS + sync + frame log
   │   ├── GnssPanel.qml          ← coordinate display
   │   ├── SyncPanel.qml          ← sync metrics grid
   │   └── FrameLog.qml           ← scrollable frame log
   ├── styles/
   │   └── colors.js              ← shared colour palette (QML)
   └── package/
       ├── AndroidManifest.xml    ← app permissions and metadata
       └── res/
           └── values/
               └── strings.xml

AndroidManifest.xml
---------------------

.. code-block:: xml

   <?xml version="1.0" encoding="utf-8"?>
   <manifest xmlns:android="http://schemas.android.com/apk/res/android"
       package="com.shrproject.cameracontrol"
       android:versionCode="1"
       android:versionName="1.0">

     <uses-sdk
         android:minSdkVersion="26"
         android:targetSdkVersion="34"/>

     <!-- USB networking permission -->
     <uses-permission android:name="android.permission.INTERNET"/>
     <uses-permission android:name="android.permission.ACCESS_NETWORK_STATE"/>
     <uses-permission android:name="android.permission.CHANGE_NETWORK_STATE"/>

     <!-- Keep screen on during acquisition -->
     <uses-permission android:name="android.permission.WAKE_LOCK"/>

     <application
         android:label="SHR Camera Control"
         android:icon="@drawable/icon"
         android:hardwareAccelerated="true"
         android:theme="@android:style/Theme.DeviceDefault.NoActionBar">

       <activity
           android:name="org.qtproject.qt.android.bindings.QtActivity"
           android:configChanges="orientation|uiMode|screenLayout|
                                  screenSize|smallestScreenSize|
                                  layoutDirection|locale|fontScale|
                                  keyboard|keyboardHidden|navigation"
           android:exported="true"
           android:screenOrientation="landscape">
         <intent-filter>
           <action android:name="android.intent.action.MAIN"/>
           <category android:name="android.intent.category.LAUNCHER"/>
         </intent-filter>
       </activity>
     </application>
   </manifest>

Building the APK
-----------------

**From Qt Creator:**

1. Select the ``SHR_Camera_Android`` target
2. Set kit to ``Android Qt 6.8 ARM64-v8a``
3. Build → Deploy to connected Android device

**From the command line:**

.. code-block:: bash

   # Set up environment
   export ANDROID_SDK_ROOT=~/Android/Sdk
   export ANDROID_NDK_ROOT=~/Android/Sdk/ndk/26.3.11579264
   export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64

   # Configure
   cmake -S . -B build-android \
     -DCMAKE_SYSTEM_NAME=Android \
     -DCMAKE_ANDROID_ARCH_ABI=arm64-v8a \
     -DCMAKE_ANDROID_NDK=$ANDROID_NDK_ROOT \
     -DANDROID_PLATFORM=android-26 \
     -DCMAKE_PREFIX_PATH=~/Qt/6.8.0/android_arm64_v8a

   # Build
   cmake --build build-android --target SHR_Camera_Android

   # Install APK to connected tablet
   adb install build-android/android-build/SHR_Camera_Android.apk

Tablet Requirements
--------------------

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Requirement
     - Notes
   * - Android 13 or higher
     - Required for native USB NCM support
   * - USB-C port
     - For USB NCM network link to V3000
   * - USB OTG support
     - Most modern tablets — verify in specs
   * - Screen size
     - 10" or larger recommended for the dual-camera layout
   * - Landscape orientation
     - App targets landscape; lock rotation in Android settings

Recommended tablets for this application:

- Samsung Galaxy Tab S9 (Android 13+, USB-C, USB OTG)
- Lenovo Tab P12 Pro (Android 12+, USB-C)
- Any rugged Android tablet (Panasonic Toughpad, Zebra, etc.) running Android 13+

.. note::
   For aviation use, check that the tablet is approved for use in your
   regulatory environment. Many operators already carry an Android tablet
   running ForeFlight or similar EFB software — this app can run
   alongside it.
