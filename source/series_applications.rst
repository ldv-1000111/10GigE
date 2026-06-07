Typical Applications
=====================

The SHR series is designed for tasks where conventional high-resolution cameras
fall short — either in resolution, image quality, or both. Below are the primary
application domains and which SHR model best fits each.

Semiconductor & Wafer Inspection
----------------------------------

Semiconductor manufacturing requires imaging entire wafers (up to 300 mm diameter)
at micron-level feature resolution to detect defects, particulates, and process
deviations.

**Why SHR:** The combination of large sensor format, high resolution, and large
pixels (high sensitivity / low noise) enables a single-shot full-wafer image
with defect detectability down to the micron scale.

**Recommended models:**

- shr461MXGE / shr461MCX — 101 MP, monochrome, largest pixel for best SNR
- shr411MXGE / shr411MCX — 151 MP for finer spatial sampling

Flat Panel Display Inspection
-------------------------------

Display manufacturers inspect LCD, OLED, and micro-LED panels at every stage
of production. Full-panel imaging in a single shot eliminates stitching artefacts
and reduces cycle time.

**Why SHR:** The large sensor covers full panel area; TEC-cooled models maintain
spectral stability for accurate colour measurement.

**Recommended models:**

- shr411CXGE-T / shr411MXGE-T — 151 MP with TEC cooling for colour stability
- shr811CCX12 — 245 MP for the largest panels or finest pixel pitch displays

Solar Panel Quality Control
-----------------------------

Solar cell and panel inspection requires detecting micro-cracks, finger interruptions,
and cell interconnection defects across the full panel area.

**Why SHR:** High resolution + high dynamic range enables detection of subtle
electroluminescence variations across the full panel.

**Recommended models:**

- shr461CXGE / shr461MXGE — 101 MP, excellent dynamic range
- shr411CXGE / shr411MXGE — 151 MP for larger panels

Aerial & Geospatial Imaging
------------------------------

Aerial cameras for mapping, surveillance, and precision agriculture require
maximum ground resolution per flight pass, minimising the number of passes needed
to cover a target area.

**Why SHR:** Highest resolution in a single sensor → maximum ground coverage
per image → fewer flight lines → lower operating cost.

**Recommended models:**

- shr811CCX12 — 245 MP, maximum coverage per frame
- shr411CXGE — 151 MP, 10GigE simplifies airborne integration

High-Speed Print & Web Inspection
-----------------------------------

Print quality control on high-speed presses requires cameras that can freeze
motion without distortion.

**Why SHR:** The shr661 is the only SHR model with a **global shutter**,
enabling distortion-free imaging of fast-moving printed substrates.

**Recommended models:**

- shr661CCX12 — 127 MP, global shutter, 20.3 fps via CXP-12
- shr661CXGE — 127 MP, global shutter, 8.2 fps via 10GigE

Scientific & Life Science Imaging
------------------------------------

Microscopy, pathology, and other scientific imaging applications require high
spatial resolution, low noise, and often long integration times.

**Why SHR:** Large pixels, low dark current, high dynamic range, and up to 82 dB
dynamic range make SHR suitable for fluorescence, brightfield, and other
scientific modalities.

**Recommended models:**

- shr461 — largest pixel (3.76 µm), best photon collection
- shr411-T — 151 MP with TEC cooling for long-exposure stability

Application vs. Model Summary
-------------------------------

.. list-table::
   :header-rows: 1
   :widths: 35 15 15 15 20

   * - Application
     - shr461 (101 MP)
     - shr661 (127 MP)
     - shr411 (151 MP)
     - shr811 (245 MP)
   * - Wafer inspection
     - ✓✓
     -
     - ✓
     -
   * - Display inspection
     -
     -
     - ✓✓ (-T)
     - ✓
   * - Solar inspection
     - ✓✓
     -
     - ✓
     -
   * - Aerial mapping
     -
     -
     - ✓
     - ✓✓
   * - Print / web inspection
     -
     - ✓✓ (GS)
     -
     -
   * - Scientific imaging
     - ✓✓
     -
     - ✓ (-T)
     -

*✓✓ = best fit, ✓ = suitable, GS = global shutter advantage, -T = TEC cooled*
