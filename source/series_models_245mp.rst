245.8 MP — shr811 Series (Sony IMX811)
========================================

The shr811 is the flagship of the SHR series and the highest-resolution
industrial camera in the Allied Vision / SVS-Vistek lineup, delivering a
staggering **245.8 megapixels** on a Sony IMX811 sensor. At this resolution,
a CoaXPress-12 interface is required to achieve useful frame rates.

Sensor Characteristics
-----------------------

.. list-table::
   :widths: 40 60

   * - Sensor
     - Sony IMX811
   * - Resolution
     - 19200 × 12800 pixels
   * - Megapixels
     - 245.8 MP
   * - Pixel size
     - 2.81 µm × 2.81 µm
   * - Sensor format
     - 53.95 mm diagonal
   * - Shutter type
     - Rolling shutter
   * - Bit depth
     - 16-bit

.. note::
   The IMX811 uses 2.81 µm pixels — the smallest in the SHR lineup. While
   smaller than the IMX461/IMX411 (3.76 µm), the Sony IMX811 is a modern
   BSI sensor with advanced noise reduction circuitry that maintains excellent
   image quality despite the smaller pixel pitch.

Models
-------

.. list-table::
   :header-rows: 1
   :widths: 25 20 20 15 20

   * - Model
     - Interface
     - Colour
     - fps
     - Notes
   * - shr811CCX12
     - CoaXPress-12
     - Color
     - 12.4
     - CXP-12 only
   * - shr811MCX12
     - CoaXPress-12
     - Monochrome
     - 12.4
     - CXP-12 only

Why CoaXPress-12 Only?
-----------------------

At 245.8 MP and 16-bit depth, a single raw frame is approximately **472 MB**.
At 12.4 fps, the required data rate is:

.. code-block:: text

   472 MB × 12.4 fps = ~5.85 GB/s

A single 10GigE link provides 1.25 GB/s — insufficient by nearly 5×. CoaXPress-12
supports up to **12.5 Gbps per channel**, and multi-channel CXP configurations
can aggregate the bandwidth needed for this sensor.

.. note::
   The shr811 requires a **CoaXPress-12 frame grabber** (PCIe). This is the
   only SHR model that requires additional frame grabber hardware. The 10GigE
   SHR models (shr461, shr661, shr411) connect directly to a NIC.

Frame Size and Bandwidth
-------------------------

.. list-table::
   :widths: 50 50

   * - Raw frame size (16-bit mono)
     - ~472 MB
   * - Raw frame size (16-bit Bayer)
     - ~472 MB
   * - RGB8 output after debayering
     - ~708 MB
   * - Data rate at 12.4 fps
     - ~5.85 GB/s (requires multi-channel CXP-12)
   * - RAM for 5 frame buffers
     - ~2.36 GB

Typical Applications
---------------------

- **Large-area flat panel display inspection** — single-shot full display coverage
- **High-resolution aerial imaging** — maximum ground resolution
- **Scientific imaging** — astronomy, microscopy mosaic replacement
- **Film / broadcast** — digital cinema capture at extreme resolution

SDK Notes — CXP via Vimba X
------------------------------

The shr811 is accessed via Vimba X using the CoaXPress transport layer,
not the GigE transport layer. The application code is identical — only
the TL installation and camera connection method differ.

Install the CXP transport layer instead of (or alongside) the GigE TL:

.. code-block:: bash

   # CoaXPress TL is provided via eGrabber (Euresys)
   # Install eGrabber from https://www.euresys.com/Products/eGrabber
   # then Vimba X will auto-detect it

Connecting to the shr811 by serial number:

.. code-block:: cpp

   CameraPtr camera;
   // CoaXPress cameras are typically identified by serial number
   system.OpenCameraByID("12345678", VmbAccessModeFull, camera);

All feature access, frame observer, and image transform code is identical
to the 10GigE models — only the pixel dimensions and buffer sizes differ:

.. code-block:: cpp

   // shr811 full resolution: 19200 × 12800
   VmbInt64_t maxW = 0, maxH = 0;
   getInt(camera, "WidthMax",  maxW);  // 19200
   getInt(camera, "HeightMax", maxH);  // 12800

   // Always use GetPayloadSize for buffer allocation
   VmbUint32_t payloadSize = 0;
   camera->GetPayloadSize(payloadSize);
   // At 12-bit packed: ~225 MB; at 16-bit: ~472 MB

   // RGB8 output buffer: 19200 × 12800 × 3 = ~708 MB
   std::vector<uint8_t> rgb(maxW * maxH * 3);
