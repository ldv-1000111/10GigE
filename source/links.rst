.. _links:

External Links & Resources
============================

Hardware
---------

**SolidRun Bedrock V3000 Basic**
   Product page, specifications, configuration selector, and resellers:
   https://www.solid-run.com/industrial-computers/bedrock-v3000-basic/

   Technical documentation:
   https://dev.solid-run.com/amd/v3000/sbc-platform/bedrock-v3000-technical-documentation

   Configuration selector (configure-to-order):
   https://dev.solid-run.com/amd/v3000/sbc-platform/bedrock-v3000-technical-documentation/specifications-bedrock-v3000/bedrock-v3000-configuration-selector

   Product brief (PDF):
   https://www.solid-run.com/wp-content/uploads/2024/10/Bedrock-V3000-Product-Brief-2024.pdf

**SolidRun Bedrock R8000**
   Product page (for comparison / AI inference node):
   https://www.solid-run.com/industrial-computers/bedrock-r8000/

   Configuration selector:
   https://dev.solid-run.com/amd/r8000-r7000/sbc-platform/bedrock-r8000-technical-documentation/specifications-bedrock-r8000/bedrock-r8000-configuration-selector

**SVS-Vistek SHR Camera Series**
   Allied Vision SHR 10GigE product page:
   https://www.alliedvision.com/en/products/area-scan-cameras/shr/shr-10gige

   Allied Vision SHR series (all interfaces):
   https://www.alliedvision.com/en/products/area-scan-cameras/shr

   SVS-Vistek SHR documentation and downloads:
   https://www.alliedvision.com/en/support/camera-documentation/area-scan-cameras-documentation/shr-documentation

   1stVision SHR family listing (all models with specs):
   https://www.1stvision.com/cameras/family/SVS-Vistek/SHR-super-high-resolution-cameras

Individual SHR 10GigE Models
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Model
     - Link
   * - shr461CXGE (101 MP color)
     - https://www.1stvision.com/cameras/models/SVS-Vistek/shr461CXGE
   * - shr461MXGE (101 MP mono)
     - https://www.1stvision.com/cameras/models/SVS-Vistek/shr461MXGE
   * - shr661CXGE (127 MP color)
     - https://www.1stvision.com/cameras/models/SVS-Vistek/shr661CXGE
   * - shr661MXGE (127 MP mono)
     - https://www.1stvision.com/cameras/models/SVS-Vistek/shr661MXGE
   * - shr411CXGE (151 MP color)
     - https://www.1stvision.com/cameras/models/SVS-Vistek/shr411CXGE
   * - shr411MXGE (151 MP mono)
     - https://www.1stvision.com/cameras/models/SVS-Vistek/shr411MXGE
   * - shr411CXGE-T (151 MP color, TEC)
     - https://www.1stvision.com/cameras/models/SVS-Vistek/shr411CXGE-T
   * - shr411MXGE-T (151 MP mono, TEC)
     - https://www.1stvision.com/cameras/models/SVS-Vistek/shr411MXGE-T

Software
---------

**Vimba X SDK**
   Downloads page (Linux x86_64, Windows):
   https://www.alliedvision.com/en/support/software-downloads/vimba-x-sdk/vimba-x

   Developer documentation (online):
   https://docs.alliedvision.com/Vimba_X/

   C++ API reference:
   https://docs.alliedvision.com/Vimba_X/VimbaXAPIManual/cpp/index.html

   Image Transform Library reference:
   https://docs.alliedvision.com/Vimba_X/VimbaXAPIManual/imageTransform/index.html

   GitHub (examples, VmbPy, GStreamer plugin):
   https://github.com/alliedvision

**Qt Framework**
   Qt 6 download (open source):
   https://www.qt.io/download-qt-installer

   Qt 6 documentation:
   https://doc.qt.io/qt-6/

   Qt for Linux / X11:
   https://doc.qt.io/qt-6/linux.html

   Qt Creator IDE:
   https://www.qt.io/product/development-tools

**Supporting Tools**
   CMake download:
   https://cmake.org/download/

   GigE Vision standard (AIA):
   https://www.visiononline.org/vision-standards-details.cfm/vision-standards/GigE-Vision/id/17

   GenICam standard:
   https://www.emva.org/standards-technology/genicam/

Sensors & Components
---------------------

**Sony IMX Sensors (SHR series)**

.. list-table::
   :header-rows: 1
   :widths: 20 20 60

   * - Sensor
     - Used in
     - Datasheet / Flyer
   * - Sony IMX461
     - shr461 (101 MP)
     - https://www.1stvision.com/cameras/sensor_specs/IMX461ALR_AQR_Flyer.pdf
   * - Sony IMX661
     - shr661 (127 MP)
     - https://www.1stvision.com/cameras/sensor_specs/IMX661-AAMR_Flyer.pdf
   * - Sony IMX411
     - shr411 (151 MP)
     - https://www.1stvision.com/cameras/sensor_specs/IMX411ALR_AQR_Flyer.pdf
   * - Sony IMX811
     - shr811 (245 MP)
     - https://www.1stvision.com/cameras/sensor_specs/IMX811-AAQR_Flyer.pdf

**AMD Ryzen Embedded V3000**
   AMD product page:
   https://www.amd.com/en/products/embedded/ryzen-embedded-v3000-series.html

   SolidRun V3000 SOM page:
   https://www.solid-run.com/embedded-networking/amd-ryzen-embedded-v3000-som/

**Hailo AI Accelerators** (for R8000 AI configurations)
   Hailo-8 (26 TOPS):
   https://hailo.ai/products/ai-accelerators/hailo-8-m2-ai-acceleration-module/

   Hailo-10 (40 TOPS, Generative AI):
   https://hailo.ai/products/ai-accelerators/hailo-10/

Standards & Protocols
----------------------

**GigE Vision**
   The network protocol used by all SHR 10GigE cameras:
   https://www.visiononline.org/vision-standards-details.cfm/vision-standards/GigE-Vision/id/17

**CoaXPress**
   Used by shr811 (245 MP) and CXP variants:
   https://www.coaxpress.com/

**GenICam**
   The camera self-description standard that Vimba X is built on:
   https://www.emva.org/standards-technology/genicam/

**IEEE 1588 PTP** (Precision Time Protocol)
   Multi-camera synchronisation via 10GigE:
   https://standards.ieee.org/ieee/1588/6825/
