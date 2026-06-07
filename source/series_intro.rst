Introduction to the SHR Series
================================

The SVS-Vistek **SHR (Super High Resolution)** cameras are the highest-resolution
area-scan camera line in the Allied Vision / TKH Vision ecosystem. Designed
for applications where no pixel of detail can be missed, the SHR series pushes
industrial imaging to its physical limits.

.. figure:: _static/shr_camera.png
   :align: center
   :alt: SHR 10GigE camera

   *SHR 10GigE camera with M72 lens mount*

What Makes the SHR Different
------------------------------

Most industrial cameras trade pixel size for resolution — cramming more, smaller
pixels onto a given sensor area. The SHR takes the opposite approach: it uses
**large pixels on large-format sensors**.

The physics behind this are clear:

- Larger pixels capture more photons per exposure → higher sensitivity
- Higher sensitivity → better signal-to-noise ratio
- Better SNR → greater dynamic range and cleaner images at lower light levels
- Large sensor format → large field of view without compromising pixel quality

The result is a camera that delivers not just high resolution, but **high image
quality at high resolution** — a combination that competing approaches cannot
achieve simultaneously.

Core Design Principles
-----------------------

**Sensor precision**
   Every SHR camera is assembled with the highest structural precision in sensor
   adjustment. Sensor flatness and tilt are controlled to fractions of a micron,
   ensuring a homogeneous image across the full sensor area.

**Thermal stability**
   Large sensors are sensitive to temperature gradients. The SHR housing is
   thermally optimized to minimize sensor temperature variation during operation.
   Selected models add active TEC (thermoelectric cooling) for the most demanding
   metrology tasks.

**Shading and defect correction**
   User-defined shading correction and custom defect pixel correction are built
   into every model, compensating for optical and sensor non-uniformities across
   the large sensor area.

**M72 lens mount**
   The large M72 thread (72 mm diameter, 19.55 mm flange distance) accommodates
   the large-format lenses required to cover these sensor sizes. Adapters are
   available for most professional lens formats.

**No frame grabber required**
   The 10GigE interface delivers up to **1.25 GB/s** over standard Cat 6 Ethernet,
   connecting directly to a host PC NIC — no specialist frame grabber hardware
   or PCIe slots needed.

Where SVS-Vistek Fits
----------------------

The SHR series is manufactured by **SVS-Vistek GmbH**, a TKH Vision company and
sister brand to Allied Vision. Since the Vimba X 2024-4 release, SVS-Vistek
cameras are natively supported by the `Vimba X SDK
<https://www.alliedvision.com/en/support/software-downloads/vimba-x-sdk/vimba-x>`_
— the same SDK used for Allied Vision cameras — enabling mixed configurations
in the same application.

.. code-block:: text

   TKH Vision Group
   ├── Allied Vision          (GigE, USB, embedded cameras)
   ├── SVS-Vistek             (HR, SHR, FXO, EXO, ECO series)
   ├── Chromasens             (line scan)
   └── NET New Electronic     (GigEPRO)

   All supported by a single SDK: Vimba X

Series Positioning
-------------------

Within the SVS-Vistek lineup, the SHR sits at the top of the resolution
hierarchy:

.. list-table::
   :header-rows: 1
   :widths: 15 25 60

   * - Series
     - Resolution range
     - Positioning
   * - ECO / ECO²
     - Low MP
     - Compact, economic GigE cameras
   * - EXO
     - Low–Mid MP
     - Multi-interface, versatile
   * - FXO
     - Mid MP
     - Sony Pregius S, CoaXPress-12 / 10GigE
   * - HR
     - 16–122 MP
     - High resolution, high frame rate
   * - **SHR**
     - **101–245 MP**
     - **Super High Resolution, highest image quality**

Key Specifications (All Models)
---------------------------------

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Feature
     - Value
   * - Resolution range
     - 101.8 MP to 245.8 MP
   * - Sensor technology
     - Sony CMOS (Pregius / IMX series)
   * - Shutter type
     - Rolling (IMX461, IMX411) / Global (IMX661)
   * - Bit depth
     - 14-bit (IMX661) or 16-bit (all others)
   * - Interface options
     - 10GigE Vision, CoaXPress
   * - 10GigE bandwidth
     - Up to 1.25 GB/s
   * - Lens mount
     - M72 × 0.75, 19.55 mm flange distance
   * - I/O
     - Industrial TTL-24V with SafeTrigger, sequencer, timer
   * - Strobe controller
     - Integrated 4-channel LED
   * - Color options
     - Color (C) and Monochrome (M) for all models
   * - TEC cooling
     - Available on selected 151 MP models (-T suffix)
   * - Cleanroom suitability
     - Yes
   * - Dynamic range
     - Up to 82 dB
   * - Color depth
     - Up to 16 bit
