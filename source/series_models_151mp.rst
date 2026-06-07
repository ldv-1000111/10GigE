151 MP — shr411 Series (Sony IMX411)
======================================

The shr411 delivers **151 megapixels** — the highest resolution available over
a standard 10GigE interface. Built on the **Sony IMX411**, it shares the same
3.76 µm pixel as the shr461 but packs it into a larger sensor array.
Selected models add **TEC (thermoelectric cooling)** for applications where
absolute sensor temperature stability is critical.

Sensor Characteristics
-----------------------

.. list-table::
   :widths: 40 60

   * - Sensor
     - Sony IMX411
   * - Resolution
     - 14192 × 10640 pixels
   * - Megapixels
     - 151.0 MP
   * - Pixel size
     - 3.76 µm × 3.76 µm
   * - Sensor format
     - Medium format (larger than 55 mm)
   * - Shutter type
     - Rolling shutter
   * - Bit depth
     - 16-bit

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
   * - shr411CXGE
     - 10GigE
     - Color
     - 6.1
     - Standard
   * - shr411MXGE
     - 10GigE
     - Monochrome
     - 6.1
     - Standard
   * - shr411CXGE-T
     - 10GigE
     - Color
     - 6.1
     - TEC cooled
   * - shr411MXGE-T
     - 10GigE
     - Monochrome
     - 6.1
     - TEC cooled
   * - shr411CCX
     - CoaXPress-6
     - Color
     - 6.1
     - Standard
   * - shr411MCX
     - CoaXPress-6
     - Monochrome
     - 6.1
     - Standard

TEC Cooling — The ``-T`` Models
---------------------------------

The shr411CXGE-T and shr411MXGE-T incorporate a **thermoelectric cooler (TEC)**
combined with an innovative ventilation system that actively maintains a stable
sensor temperature.

Why this matters:

- As sensor temperature rises, dark current increases → more noise
- Spectral sensitivity shifts with temperature → colour accuracy degrades
- These effects are especially problematic in **colour display inspection**,
  where precise colour calibration is essential

The TEC models hold sensor temperature constant regardless of ambient conditions,
delivering repeatable, calibration-stable images over long production runs.

.. note::
   The -T models are recommended for:

   - Flat panel display colour inspection (requires stable spectral response)
   - High-precision colour metrology
   - Environments with variable ambient temperature
   - Any application where results must be reproducible hours or days apart

Frame Size and Bandwidth
-------------------------

At 151 MP and 16-bit depth:

.. list-table::
   :widths: 50 50

   * - Raw frame size (16-bit mono)
     - ~288 MB
   * - Raw frame size (16-bit Bayer)
     - ~288 MB
   * - RGB8 output after debayering
     - ~432 MB
   * - 10GigE at 6.1 fps
     - ~1.10 GB/s (near the 1.25 GB/s limit)
   * - RAM for 5 frame buffers
     - ~1.44 GB

.. warning::
   At 151 MP, a single RGB8 output frame is ~432 MB. If saving frames to disk,
   ensure your storage can sustain the write speed. An NVMe SSD is strongly
   recommended; spinning drives will be the bottleneck within seconds.

Typical Applications
---------------------

- **Semiconductor wafer inspection** — full wafer in one shot at fine resolution
- **Flat panel display inspection** — full panel coverage, pixel-level fidelity
- **Aerial / satellite ground station imaging**
- **Large-area scientific imaging** — life science, geo mapping

SDK Notes
----------

Full resolution and buffer sizing for the shr411:

.. code-block:: cpp

   // shr411 full resolution: 14192 × 10640
   setEnum(camera, "PixelFormat", "BayerGR12");   // Color, 12-bit Bayer
   // or
   setEnum(camera, "PixelFormat", "Mono12");       // Monochrome

   // Get max resolution
   VmbInt64_t maxW = 0, maxH = 0;
   getInt(camera, "WidthMax",  maxW);  // 14192
   getInt(camera, "HeightMax", maxH);  // 10640
   setInt(camera, "Width",  maxW);
   setInt(camera, "Height", maxH);

   // Buffer size — always use GetPayloadSize(), not manual calculation
   VmbUint32_t payloadSize = 0;
   camera->GetPayloadSize(payloadSize);
   // At 12-bit packed Bayer: ~160 MB per frame
   // At 16-bit Bayer:        ~288 MB per frame

   // For 5 buffers at 12-bit packed: ~800 MB RAM needed
   FramePtrVector frames(5);
   for (auto& f : frames)
   {
       f.reset(new Frame(payloadSize));
       f->RegisterObserver(observer);
       camera->AnnounceFrame(f);
   }
