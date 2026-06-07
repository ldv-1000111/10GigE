Model Overview & Naming Convention
====================================

The SHR series currently comprises **four sensor variants**, each available
in multiple interface and colour configurations. Together they span
**101.8 MP to 245.8 MP**. Full listings are available on the
`Allied Vision SHR page
<https://www.alliedvision.com/en/products/area-scan-cameras/shr/shr-10gige>`_
and the `1stVision SHR family page
<https://www.1stvision.com/cameras/family/SVS-Vistek/SHR-super-high-resolution-cameras>`_.

Model Naming Convention
------------------------

SHR model names follow a consistent pattern:

.. code-block:: text

   shr  [sensor]  [C|M]  [XGE|CX|CX12]  [-T]
    │      │        │         │             │
    │      │        │         │             └─ TEC cooled (optional)
    │      │        │         └─ Interface: XGE=10GigE, CX=CXP-6, CX12=CXP-12
    │      │        └─ Colour: C=Color, M=Monochrome
    │      └─ Sony IMX sensor number (461, 661, 411, 811)
    └─ Super High Resolution series

Examples:

- ``shr461CXGE``  — 101 MP, Color, 10GigE
- ``shr461MXGE``  — 101 MP, Monochrome, 10GigE
- ``shr411CXGE-T``— 151 MP, Color, 10GigE, TEC cooled
- ``shr811CCX12`` — 245 MP, Color, CoaXPress-12

Complete Model Matrix
----------------------

.. list-table::
   :header-rows: 1
   :widths: 18 12 18 10 12 10 10 10

   * - Model
     - MP
     - Resolution
     - Sensor
     - Interface
     - fps
     - Bit depth
     - Shutter
   * - shr461CXGE
     - 101.8
     - 11648×8742
     - IMX461
     - 10GigE
     - 8.7
     - 16
     - Rolling
   * - shr461MXGE
     - 101.8
     - 11648×8742
     - IMX461
     - 10GigE
     - 8.7
     - 16
     - Rolling
   * - shr461CCX
     - 101.8
     - 11648×8742
     - IMX461
     - CXP-6
     - 8.7
     - 16
     - Rolling
   * - shr461MCX
     - 101.8
     - 11648×8742
     - IMX461
     - CXP-6
     - 8.7
     - 16
     - Rolling
   * - shr661CXGE
     - 127.6
     - 13392×9528
     - IMX661
     - 10GigE
     - 8.2
     - 14
     - Global
   * - shr661MXGE
     - 127.6
     - 13392×9528
     - IMX661
     - 10GigE
     - 8.2
     - 14
     - Global
   * - shr661CCX12
     - 127.6
     - 13392×9528
     - IMX661
     - CXP-12
     - 20.3
     - 14
     - Global
   * - shr661MCX12
     - 127.6
     - 13392×9528
     - IMX661
     - CXP-12
     - 20.3
     - 14
     - Global
   * - shr411CXGE
     - 151.0
     - 14192×10640
     - IMX411
     - 10GigE
     - 6.1
     - 16
     - Rolling
   * - shr411MXGE
     - 151.0
     - 14192×10640
     - IMX411
     - 10GigE
     - 6.1
     - 16
     - Rolling
   * - shr411CXGE-T
     - 151.0
     - 14192×10640
     - IMX411
     - 10GigE
     - 6.1
     - 16
     - Rolling
   * - shr411MXGE-T
     - 151.0
     - 14192×10640
     - IMX411
     - 10GigE
     - 6.1
     - 16
     - Rolling
   * - shr411CCX
     - 151.0
     - 14192×10640
     - IMX411
     - CXP-6
     - 6.1
     - 16
     - Rolling
   * - shr411MCX
     - 151.0
     - 14192×10640
     - IMX411
     - CXP-6
     - 6.1
     - 16
     - Rolling
   * - shr811CCX12
     - 245.8
     - 19200×12800
     - IMX811
     - CXP-12
     - 12.4
     - 16
     - Rolling
   * - shr811MCX12
     - 245.8
     - 19200×12800
     - IMX811
     - CXP-12
     - 12.4
     - 16
     - Rolling

.. note::
   The 245.8 MP (IMX811) models are **CoaXPress-12 only**. At 245 MP the raw
   data rate exceeds what a single 10GigE link can sustain at useful frame
   rates — CoaXPress-12 provides the necessary bandwidth.

Choosing a Model
-----------------

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - If you need...
     - Choose...
   * - Widest field of view, highest resolution
     - shr811 (245 MP, CXP-12)
   * - Highest resolution over standard Ethernet
     - shr411 (151 MP, 10GigE)
   * - Global shutter at ultra-high resolution
     - shr661 (127 MP) — only SHR model with global shutter
   * - Best SNR / largest pixels at ~100 MP
     - shr461 (101 MP, 3.76 µm pixel)
   * - Thermally stabilised sensor for colour metrology
     - shr411CXGE-T or shr411MXGE-T (TEC cooled)
   * - Fastest frame rate at high resolution
     - shr661CCX12 (20.3 fps via CXP-12)
