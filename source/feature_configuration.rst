Feature Configuration
=====================

All camera settings in Vimba X are controlled through **features** — a
GenICam-standard mechanism. The camera provides a self-describing XML file
that Vimba X reads automatically, so every feature — including vendor-specific
SHR features — is accessible without SDK updates.

Feature Types
-------------

.. list-table::
   :header-rows: 1
   :widths: 20 30 50

   * - Type
     - C++ Access
     - Example features
   * - Integer
     - ``GetValue(VmbInt64_t&)`` / ``SetValue(VmbInt64_t)``
     - ``Width``, ``Height``, ``GVSPBurstSize``
   * - Float
     - ``GetValue(double&)`` / ``SetValue(double)``
     - ``ExposureTime``, ``Gain``
   * - Enum
     - ``GetValue(std::string&)`` / ``SetValue(std::string)``
     - ``PixelFormat``, ``AcquisitionMode``, ``TriggerSource``
   * - Command
     - ``RunCommand()``
     - ``AcquisitionStart``, ``AcquisitionStop``
   * - Bool
     - ``GetValue(bool&)`` / ``SetValue(bool)``
     - Various enable flags
   * - String
     - ``GetValue(std::string&)``
     - ``DeviceModelName``

Reading and Writing Features
-----------------------------

The pattern is always: get a ``FeaturePtr`` by name, then call the typed
accessor.

**Reading an integer feature:**

.. code-block:: cpp

   FeaturePtr feature;
   VmbInt64_t width = 0;

   if (VmbErrorSuccess == camera->GetFeatureByName("Width", feature))
       feature->GetValue(width);

   std::cout << "Width: " << width << std::endl;

**Writing an enum feature:**

.. code-block:: cpp

   FeaturePtr feature;
   if (VmbErrorSuccess == camera->GetFeatureByName("PixelFormat", feature))
       feature->SetValue("BayerGR12");

**Running a command feature:**

.. code-block:: cpp

   FeaturePtr feature;
   if (VmbErrorSuccess == camera->GetFeatureByName("AcquisitionStart", feature))
       feature->RunCommand();

Helper Functions
-----------------

To avoid repetitive error-checking boilerplate, define helpers:

.. code-block:: cpp

   VmbError_t setEnum(CameraPtr& cam, const char* name, const char* value)
   {
       FeaturePtr f;
       VmbError_t e = cam->GetFeatureByName(name, f);
       if (e != VmbErrorSuccess) return e;
       return f->SetValue(value);
   }

   VmbError_t setInt(CameraPtr& cam, const char* name, VmbInt64_t value)
   {
       FeaturePtr f;
       VmbError_t e = cam->GetFeatureByName(name, f);
       if (e != VmbErrorSuccess) return e;
       return f->SetValue(value);
   }

   VmbError_t runCmd(CameraPtr& cam, const char* name)
   {
       FeaturePtr f;
       VmbError_t e = cam->GetFeatureByName(name, f);
       if (e != VmbErrorSuccess) return e;
       return f->RunCommand();
   }

SHR-Specific Configuration
----------------------------

Pixel Format
^^^^^^^^^^^^

The SHR 10GigE typically outputs 12-bit Bayer data. Set the pixel format
before starting acquisition:

.. code-block:: cpp

   setEnum(camera, "PixelFormat", "BayerGR12");

Common SHR pixel formats:

- ``BayerGR12`` — 12-bit Bayer (most common for color SHR models)
- ``BayerGR8``  — 8-bit Bayer (lower bandwidth, less dynamic range)
- ``Mono12``    — 12-bit monochrome (monochrome SHR models)
- ``Mono8``     — 8-bit monochrome

Full Sensor Resolution
^^^^^^^^^^^^^^^^^^^^^^^

Read the maximum resolution from the camera and apply it:

.. code-block:: cpp

   VmbInt64_t maxWidth = 0, maxHeight = 0;
   FeaturePtr f;

   camera->GetFeatureByName("WidthMax", f);
   f->GetValue(maxWidth);

   camera->GetFeatureByName("HeightMax", f);
   f->GetValue(maxHeight);

   setInt(camera, "Width",  maxWidth);
   setInt(camera, "Height", maxHeight);

   std::cout << "Resolution set to: "
             << maxWidth << " x " << maxHeight << std::endl;

.. tip::
   For a 245 MP SHR model, ``maxWidth`` × ``maxHeight`` will be approximately
   17,000 × 14,000 pixels. Each 12-bit Bayer frame is roughly **350 MB**.
   Ensure your system memory can accommodate your buffer count × frame size.

Acquisition Mode
^^^^^^^^^^^^^^^^^

For continuous streaming:

.. code-block:: cpp

   setEnum(camera, "AcquisitionMode", "Continuous");

Other options: ``SingleFrame``, ``MultiFrame``.

GigE-Specific: GVSP Burst Size
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For 10GigE cameras, increase the GVSP burst size to improve throughput.
The System Settings documentation recommends **32 or higher** for 5GigE+
cameras:

.. code-block:: cpp

   setInt(camera, "GVSPBurstSize", 64);

Checking Feature Availability
------------------------------

Not all features are always writable (some are read-only or locked during
acquisition). Check before writing:

.. code-block:: cpp

   FeaturePtr feature;
   camera->GetFeatureByName("Width", feature);

   bool writable = false;
   feature->IsWritable(writable);

   if (writable)
       feature->SetValue(newWidth);
   else
       std::cerr << "Width is not writable in current state" << std::endl;

Listing All Features
---------------------

To see every feature available on your SHR camera:

.. code-block:: cpp

   FeaturePtrVector features;
   camera->GetFeatures(features);

   for (const auto& f : features)
   {
       std::string name, displayName, category;
       f->GetName(name);
       f->GetDisplayName(displayName);
       f->GetCategory(category);
       std::cout << category << " / " << displayName
                 << " (" << name << ")" << std::endl;
   }
