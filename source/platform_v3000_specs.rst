Bedrock V3000 — Specifications & SHR Mapping
=============================================

The `SolidRun Bedrock V3000 Basic
<https://www.solid-run.com/industrial-computers/bedrock-v3000-basic/>`_
is a compact fanless industrial PC built on the **AMD Ryzen Embedded V3C48**
processor. Every key specification maps directly to a requirement of the SHR
10GigE streaming workload.

Full Specifications
--------------------

.. list-table::
   :header-rows: 1
   :widths: 30 40 30

   * - Feature
     - Specification
     - Notes
   * - **CPU**
     - AMD Ryzen Embedded V3C48
       8C/16T Zen3+, 6nm
       Up to 3.8 GHz, 45W
     - 20 lanes PCIe Gen4
   * - **RAM**
     - Quad-channel DDR5-4800
       Up to 96 GB ECC/non-ECC
     - 2× SODIMM, conduction cooled
   * - **Main storage**
     - 1× NVMe PCIe Gen4 ×4
     - M.2 2280, optional PLP
   * - **Extra storage**
     - 2× NVMe PCIe Gen4 ×4
     - M.2 2280, conduction cooled
   * - **LAN**
     - 2× **10 GbE** SFP+ (native)
       4× 2.5 GbE (Intel I226)
     - Copper or fiber
   * - **WLAN**
     - WiFi 6E (Intel AX210), BT 5.3
     - Optional, M.2 key-E
   * - **Modem**
     - 4G/5G (Quectel)
     - Optional, M.2 key-B
   * - **USB**
     - 3× USB 3.2 Gen2 (10 Gbps)
       1× USB 2.0
     - All type-A
   * - **BIOS**
     - AMI Aptio V, dual SPI Flash
     - Redundant, WDT, TPM
   * - **OS support**
     - Windows 10/11/IoT, Linux
     - x86 — full Qt/Vimba X support
   * - **Power input**
     - 12V–60V DC
     - Phoenix terminal block
   * - **Temperature**
     - -40°C to 85°C (industrial)
     - Fanless, ventless, IP40
   * - **Enclosure**
     - All-aluminium, fanless
     - DIN-rail, wall, desk mount
   * - **Dimensions (60W)**
     - 73 × 160 × 130 mm — 1.5 L
     -
   * - **Dimensions (30W)**
     - 45 × 160 × 130 mm — 0.9 L
     -

Mapping Specifications to SHR Requirements
--------------------------------------------

Native Dual 10 GbE — The Critical Differentiator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The V3000's two 10 GbE SFP+ ports are **native to the AMD Ryzen Embedded
V3000 SoC** — they share the processor's PCIe Gen4 fabric directly, not
through a PCIe add-in card or USB bridge.

.. code-block:: text

   AMD Ryzen V3C48 SoC
   ├── PCIe Gen4 × 20 lanes
   │   ├── 10 GbE SFP+ port 0  ◄── SHR camera (dedicated lane)
   │   ├── 10 GbE SFP+ port 1  ◄── second camera or uplink
   │   ├── NVMe Gen4 (main)    ◄── OS + application
   │   ├── NVMe Gen4 (slot 2)  ◄── frame storage
   │   └── NVMe Gen4 (slot 3)  ◄── overflow / RAID
   └── DDR5-4800 (quad channel) ◄── 1.25 GB/s DMA destination

This means the 10 GbE receive path has **dedicated PCIe bandwidth** that
does not compete with storage or USB. A single SHR camera can use one port
exclusively; a second SFP+ port remains available for uplink or a second camera.

ECC DDR5 — Protecting Sustained DMA
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

At 1.25 GB/s continuous DMA into system RAM, a standard DRAM bit error
(roughly 1 per 10\ :sup:`9` bits at typical error rates) would occur
approximately every **6–8 seconds** at full SHR throughput. Without ECC,
this silently corrupts frame data in memory — producing images with subtle
pixel errors that are extremely difficult to detect downstream.

ECC detects and corrects single-bit errors transparently, and detects (but
flags) double-bit errors. For production inspection systems where image
integrity is the product, ECC DDR5 is non-negotiable.

Three NVMe PCIe Gen4 Slots — Frame Storage at Speed
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A 151 MP RGB8 frame is ~432 MB. Writing frames to a single NVMe at 6.1 fps
would require ~2.6 GB/s sustained write — beyond any single consumer NVMe.

The V3000's three PCIe Gen4 slots allow:

- **Slot 1** — OS and Vimba X + application (fast, low-latency reads)
- **Slot 2** — Primary frame storage NVMe (full write bandwidth dedicated)
- **Slot 3** — Secondary frame storage or software RAID 0 for peak throughput

PCIe Gen4 ×4 per slot delivers up to 7 GB/s theoretical per device. In
practice, a high-end Gen4 NVMe (e.g. Samsung 990 Pro) sustains ~7 GB/s
sequential write — more than sufficient for full-rate SHR capture.

Fanless Industrial Design — 24/7 Operation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

SHR inspection systems run continuously. The V3000's fanless thermal design:

- Eliminates fan failure as a point of unreliability
- Removes moving parts that attract particulates (cleanroom compatible)
- Operates at -40°C to 85°C — suitable for harsh factory environments
- Uses liquid metal TIM and stacked heatpipes to dissipate over 3× the power
  of similarly-sized fanless computers

The CPU, both SODIMMs, all three NVMe drives, NICs, and SFP+ cages are all
thermally coupled to the aluminium chassis — secondary heat sources that would
cause thermal throttling in a lesser design are all managed.

Wide Voltage DC Input — Industrial Power
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The 12V–60V DC input via a screw-locking Phoenix terminal block means the
V3000 can be powered directly from industrial DC rails (24V, 48V) without
intermediate conversion, reducing points of failure and power loss.
