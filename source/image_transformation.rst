Image Transformation
====================

Raw frames from the SHR camera arrive as Bayer-pattern data (e.g. ``BayerGR12``).
The Vimba X **Image Transform Library** converts these into usable formats
such as RGB8 — without requiring knowledge of GenICam's PFNC naming scheme.

How It Works
-------------

Every transformation requires three things:

1. A fully-described **source image** (format + pointer to raw data)
2. A fully-described **destination image** (target format + output buffer)
3. Optional **transformation parameters** (debayer algorithm, color matrix)

The main function is:

.. code-block:: cpp

   VmbImageTransform(
       &sourceImage,       // Input: raw frame from camera
       &destinationImage,  // Output: transformed image
       &transformInfo,     // Optional: debayer mode, color matrix, etc.
       parameterCount      // Number of transform parameters
   );

Supported Pixel Formats
------------------------

The SHR commonly outputs these source formats:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Format
     - Description
   * - ``BayerGR8``
     - 8-bit Bayer (color models)
   * - ``BayerGR12``
     - 12-bit Bayer, LSB-aligned (most common for SHR color)
   * - ``BayerGR12p``
     - 12-bit Bayer, packed (higher bandwidth efficiency)
   * - ``Mono8``
     - 8-bit monochrome (monochrome SHR models)
   * - ``Mono12``
     - 12-bit monochrome

.. note::
   All multi-byte pixel formats must be in **little-endian** byte order.
   Bayer formats additionally require **even width and height**.

Step-by-Step: Bayer to RGB8
-----------------------------

This is the most common transformation for the SHR color series.

.. code-block:: cpp

   bool transformToRGB8(
       void*              pSrcData,
       VmbPixelFormat_t   srcFormat,
       VmbUint32_t        width,
       VmbUint32_t        height,
       std::vector<uint8_t>& outBuffer)  // Must be width * height * 3 bytes
   {
       VmbImage sourceImage{};
       VmbImage destinationImage{};

       sourceImage.Size      = sizeof(sourceImage);
       destinationImage.Size = sizeof(destinationImage);

       // Attach raw frame data
       sourceImage.Data      = pSrcData;
       destinationImage.Data = outBuffer.data();

       // Describe the source format
       VmbError_t err = VmbSetImageInfoFromPixelFormat(
           srcFormat, width, height, &sourceImage);
       if (err != VmbErrorSuccess) return false;

       // Describe the destination: RGB, 8 bits per channel
       err = VmbSetImageInfoFromInputImage(
           &sourceImage, VmbPixelLayoutRGB, 8, &destinationImage);
       if (err != VmbErrorSuccess) return false;

       // Set the debayer algorithm
       VmbTransformInfo debayerInfo{};
       VmbSetDebayerMode(VmbDebayerMode2x2, &debayerInfo);

       // Run the transformation
       err = VmbImageTransform(
           &sourceImage, &destinationImage, &debayerInfo, 1);

       return err == VmbErrorSuccess;
   }

Allocate the output buffer before calling:

.. code-block:: cpp

   std::vector<uint8_t> rgbBuffer(width * height * 3);
   transformToRGB8(pData, pixelFormat, width, height, rgbBuffer);

Debayering Algorithms
----------------------

The debayer algorithm trades processing speed for image quality:

.. list-table::
   :header-rows: 1
   :widths: 30 20 50

   * - Mode
     - Bit depths
     - Description
   * - ``VmbDebayerMode2x2``
     - All
     - 2×2 with green averaging. **Default**. Fastest.
   * - ``VmbDebayerMode3x3``
     - All
     - 3×3 with equal green weighting per line. Better quality.
   * - ``VmbDebayerModeLCAA``
     - 8-bit only
     - Horizontal local colour anti-aliasing. High quality.
   * - ``VmbDebayerModeLCAAV``
     - 8-bit only
     - Horizontal + vertical LCAA. Highest quality.
   * - ``VmbDebayerModeYUV422``
     - 8-bit only
     - YUV422-style sub-sampling. For display pipelines.

.. warning::
   ``LCAA``, ``LCAAV``, and ``YUV422`` modes use the **input buffer as
   an intermediate buffer** during processing. If you need the raw input
   data after transformation, copy it beforehand.

Color Correction
-----------------

Apply a 3×3 float matrix to correct colour response after debayering.
The matrix multiplies each output RGB pixel:

.. code-block:: text

   [ R_out ]   [ rr  rg  rb ]   [ R_in ]
   [ G_out ] = [ gr  gg  gb ] × [ G_in ]
   [ B_out ]   [ br  bg  bb ]   [ B_in ]

Example — identity matrix (no change):

.. code-block:: cpp

   VmbTransformInfo ccInfo{};
   const VmbFloat_t mat[] = {
       1.0f, 0.0f, 0.0f,
       0.0f, 1.0f, 0.0f,
       0.0f, 0.0f, 1.0f
   };
   VmbSetColorCorrectionMatrix3x3(mat, &ccInfo);

Combining Debayering and Colour Correction
-------------------------------------------

Pass multiple ``VmbTransformInfo`` entries in a single call:

.. code-block:: cpp

   VmbTransformInfo params[2];

   // Param 0: debayer
   VmbSetDebayerMode(VmbDebayerMode3x3, &params[0]);

   // Param 1: color correction
   const VmbFloat_t mat[] = { 1.0f, 0.0f, 0.0f,
                               0.0f, 1.0f, 0.0f,
                               0.0f, 0.0f, 1.0f };
   VmbSetColorCorrectionMatrix3x3(mat, &params[1]);

   VmbImageTransform(&sourceImage, &destinationImage, params, 2);

Output Format Helper Functions
--------------------------------

.. list-table::
   :header-rows: 1
   :widths: 45 55

   * - Function
     - Use when you have...
   * - ``VmbSetImageInfoFromPixelFormat()``
     - A ``VmbPixelFormat_t`` value (from frame or feature)
   * - ``VmbSetImageInfoFromInputImage()``
     - An already-described source image; derive dest format from it
   * - ``VmbSetImageInfoFromInputParameters()``
     - Explicit format enum + width + height
   * - ``VmbSetImageInfoFromString()``
     - A format name string (expert use)
