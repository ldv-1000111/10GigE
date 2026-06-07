101.8 MP — shr461 Series (Sony IMX461)
========================================

The shr461 is the entry point of the SHR line, built around the **Sony IMX461**
— a large-format backside-illuminated CMOS sensor that delivers outstanding
sensitivity and low noise at 101.8 megapixels.

Sensor Characteristics
-----------------------

.. list-table::
   :widths: 40 60

   * - Sensor
     - Sony IMX461
   * - Resolution
     - 11648 × 8742 pixels
   * - Megapixels
     - 101.8 MP
   * - Pixel size
     - 3.76 µm × 3.76 µm
   * - Sensor format
     - 55 mm diagonal
   * - Shutter type
     - Rolling shutter
   * - Bit depth
     - 16-bit
   * - Technology
     - BSI (Back-Side Illuminated) CMOS

.. note::
   The 3.76 µm pixel of the IMX461 is the **largest pixel in the SHR lineup**,
   making it the most light-sensitive option and delivering the best SNR per pixel.

Models
-------

.. list-table::
   :header-rows: 1
   :widths: 25 20 20 15 20

   * - Model
     - Interface
     - Colour
     - fps
     - Pixel size
   * - shr461CXGE
     - 10GigE
     - Color
     - 8.7
     - 3.76 µm
   * - shr461MXGE
     - 10GigE
     - Monochrome
     - 8.7
     - 3.76 µm
   * - shr461CCX
     - CoaXPress-6
     - Color
     - 8.7
     - 3.76 µm
   * - shr461MCX
     - CoaXPress-6
     - Monochrome
     - 8.7
     - 3.76 µm

Frame Size and Bandwidth
-------------------------

At full resolution and 16-bit depth:

.. list-table::
   :widths: 50 50

   * - Raw frame size (16-bit mono)
     - ~204 MB
   * - Raw frame size (16-bit Bayer)
     - ~204 MB
   * - RGB8 output after debayering
     - ~291 MB
   * - 10GigE bandwidth at 8.7 fps
     - ~1.12 GB/s (near the 1.25 GB/s limit)

Typical Applications
---------------------

- **Semiconductor wafer inspection** — large field of view, fine feature detection
- **Solar panel quality control** — full panel in a single shot
- **Flat panel display inspection** — high sensitivity for defect detection
- **Aerial mapping** — large sensor area with excellent dynamic range

SDK Notes (10GigE Models)
--------------------------

For the shr461CXGE and shr461MXGE, the pixel format when using Vimba X is:

.. code-block:: cpp

   // Color model
   setEnum(camera, "PixelFormat", "BayerGR12");  // or BayerGR16

   // Monochrome model
   setEnum(camera, "PixelFormat", "Mono12");      // or Mono16

Full resolution via Vimba X:

.. code-block:: cpp

   // shr461: 11648 x 8742
   // WidthMax and HeightMax will return these values
   VmbInt64_t maxW = 0, maxH = 0;
   getInt(camera, "WidthMax",  maxW);  // 11648
   getInt(camera, "HeightMax", maxH);  // 8742
   setInt(camera, "Width",  maxW);
   setInt(camera, "Height", maxH);

Buffer allocation for this model requires planning — at 8.7 fps with 5 buffers:

.. code-block:: text

   5 buffers × 204 MB = ~1 GB RAM for frame buffers alone
