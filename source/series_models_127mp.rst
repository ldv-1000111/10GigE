127.6 MP — shr661 Series (Sony IMX661)
========================================

The shr661 is the **only SHR model with a global shutter**, built around the
**Sony IMX661** from the Pregius series. Its backside-illuminated design
delivers exceptional light sensitivity combined with very low noise, and the
global shutter eliminates the rolling shutter distortion that affects fast-moving
subjects.

Sensor Characteristics
-----------------------

.. list-table::
   :widths: 40 60

   * - Sensor
     - Sony IMX661
   * - Resolution
     - 13392 × 9528 pixels
   * - Megapixels
     - 127.6 MP
   * - Pixel size
     - 3.45 µm × 3.45 µm
   * - Sensor format
     - 56.73 mm diagonal
   * - Shutter type
     - **Global shutter**
   * - Bit depth
     - 14-bit
   * - Technology
     - BSI CMOS (Pregius series)

.. tip::
   The shr661 is the right choice when subjects move during exposure, or when
   synchronising multiple cameras for stereo / multi-view imaging. All other
   SHR models use rolling shutter.

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
   * - shr661CXGE
     - 10GigE
     - Color
     - 8.2
     - Standard
   * - shr661MXGE
     - 10GigE
     - Monochrome
     - 8.2
     - Standard
   * - shr661CCX12
     - CoaXPress-12
     - Color
     - 20.3
     - 2.5× faster than 10GigE
   * - shr661MCX12
     - CoaXPress-12
     - Monochrome
     - 20.3
     - 2.5× faster than 10GigE

.. note::
   The CXP-12 models push 20.3 fps — the highest frame rate in the SHR series
   at any resolution. This makes the shr661CCX12/MCX12 the preferred choice
   for high-speed inspection at ultra-high resolution.

Frame Size and Bandwidth
-------------------------

At full resolution and 14-bit depth:

.. list-table::
   :widths: 50 50

   * - Raw frame size (14-bit, packed)
     - ~191 MB
   * - RGB8 output after debayering
     - ~365 MB
   * - 10GigE at 8.2 fps
     - ~0.98 GB/s
   * - CXP-12 at 20.3 fps
     - ~2.42 GB/s (requires CXP-12 frame grabber)

Typical Applications
---------------------

- **High-speed print inspection** — global shutter handles fast-moving web
- **Multi-camera 3D / stereo** — global shutter enables precise synchronisation
- **Robot-mounted inspection** — no rolling shutter distortion during motion
- **Electronics inspection** — large field with high resolution

SDK Notes — Pixel Format Difference
-------------------------------------

The IMX661 outputs 14-bit data, not 16-bit like the other SHR sensors. This
affects how you set the pixel format:

.. code-block:: cpp

   // Color shr661 — 14-bit Bayer
   setEnum(camera, "PixelFormat", "BayerGR14");

   // Monochrome shr661 — 14-bit mono
   setEnum(camera, "PixelFormat", "Mono14");

When transforming to RGB8, the Image Transform Library handles 14-bit input
the same way as 12-bit or 16-bit — the conversion to 8-bit is lossy (truncating
the lower bits):

.. code-block:: cpp

   // Source: BayerGR14, Destination: RGB8 — same API call
   VmbSetImageInfoFromPixelFormat(VmbPixelFormatBayerGR14, width, height, &src);
   VmbSetImageInfoFromInputImage(&src, VmbPixelLayoutRGB, 8, &dst);
   VmbSetDebayerMode(VmbDebayerMode2x2, &info);
   VmbImageTransform(&src, &dst, &info, 1);

To preserve the full 14-bit dynamic range, transform to a 16-bit output instead:

.. code-block:: cpp

   // Preserve full bit depth — output to Mono16 or RGB16
   VmbSetImageInfoFromInputImage(&src, VmbPixelLayoutRGB, 16, &dst);
   // Output buffer: width × height × 6 bytes (RGB16)
