V3000 vs R8000 — Platform Comparison
======================================

Both the `Bedrock V3000
<https://www.solid-run.com/industrial-computers/bedrock-v3000-basic/>`_
and `Bedrock R8000
<https://www.solid-run.com/industrial-computers/bedrock-r8000/>`_
are fanless industrial PCs from SolidRun built on AMD Ryzen Embedded
processors. For a Qt-based SHR 10GigE capture application, the choice
between them is clear — but understanding **why** is important for making
the right call in other configurations.

Specification Comparison
-------------------------

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Feature
     - Bedrock V3000 Basic
     - Bedrock R8000
   * - **CPU**
     - Ryzen Embedded V3C48
       8C/16T Zen3+, 6nm
       Up to 3.8 GHz, 45W
     - Ryzen Embedded 8000
       8C/16T Zen4, 4nm
       Up to 5.2 GHz, 54W
   * - **GPU**
     - Integrated (basic iGPU)
     - **AMD Radeon 780M**
       12 CUs @ 2800 MHz
   * - **NPU**
     - None
     - AMD Ryzen AI — 16 TOPS
   * - **AI acceleration**
     - None
     - Up to 3× Hailo modules
       26–40 TOPS each
       100+ TOPS combined
   * - **Native 10 GbE**
     - **2× 10 GbE SFP+** (native)
     - ✗ None
   * - **Max Ethernet**
     - 2× 10 GbE + 4× 2.5 GbE
     - 4× 2.5 GbE only
   * - **Display outputs**
     - Not specified
     - Up to 4× HDMI 2.1 / DP 2.1
       Up to 7680×4320 @ 60 Hz
   * - **RAM**
     - Up to 96 GB DDR5-4800 ECC
     - Up to 96 GB DDR5-5600 ECC
   * - **NVMe slots**
     - 3× PCIe Gen4 ×4
     - 3× PCIe Gen4 ×4
       (shared with Hailo M.2 slots)
   * - **USB**
     - 3× USB 3.2 Gen2
       1× USB 2.0
     - 1× USB4 40 Gbps
       4× USB 3.2
   * - **Temperature**
     - -40°C to 85°C
     - -40°C to 85°C
   * - **Form factor**
     - Fanless, DIN-rail / wall
     - Fanless, DIN-rail / wall / VESA

Why the V3000 Wins for SHR Acquisition
-----------------------------------------

The single most important factor is network architecture.

**The R8000 has no 10 GbE.** Its maximum LAN speed is 2.5 GbE per port —
a hard ceiling of 312 MB/s per port. An SHR 10GigE camera transmits up to
**1.25 GB/s** continuously. There is no configuration of the R8000's built-in
networking that can receive a single SHR camera at full rate.

Adding a PCIe 10 GbE NIC to the R8000 is possible but introduces problems:

- **PCIe bandwidth contention** — the added NIC shares PCIe lanes with
  NVMe storage and the Hailo AI modules
- **No native integration** — driver support, interrupt routing, and DMA
  performance are lower than a natively designed interface
- **Slot occupation** — the R8000's M.2 slots are shared between NVMe,
  Hailo AI cards, and WiFi; adding a PCIe NIC consumes a slot otherwise
  used for frame storage or AI acceleration
- **Design complexity** — a retrofit NIC is an additional component to
  qualify, cable, and maintain in an industrial system

The V3000's 10 GbE is not bolted on — it is part of the SoC fabric,
with dedicated PCIe Gen4 lanes, native driver support, and thermal
management built into the chassis design.

When the R8000 Would Be the Right Choice
------------------------------------------

The R8000's advantages — Radeon 780M GPU, Ryzen AI NPU, Hailo AI slots,
USB4, and 4 display outputs — become decisive in different application profiles:

.. list-table::
   :header-rows: 1
   :widths: 55 22 22

   * - Application requirement
     - V3000
     - R8000
   * - Sustained 10 GbE SHR camera stream
     - ✓✓
     - ✗
   * - Full-resolution Qt live display pipeline
     - ○
     - ✓✓
   * - On-device AI inference (defect detection)
     - ✗
     - ✓✓
   * - Multi-display inspection GUI
     - ○
     - ✓✓
   * - Qt trigger/acquisition control panel
     - ✓✓
     - ○
   * - 24/7 continuous industrial acquisition
     - ✓✓
     - ✓
   * - Cleanroom / harsh environment
     - ✓✓
     - ✓✓

*✓✓ = purpose-built fit, ✓ = suitable, ○ = adequate, ✗ = not suitable*

.. note::
   If your roadmap includes adding **on-device AI inference** (e.g. real-time
   defect detection on captured frames) alongside the SHR camera, the right
   architecture is the **V3000 for camera acquisition** connected over a local
   network to an **R8000 (or RAI300) for inference** — separating the
   throughput-critical receive path from the compute-intensive AI workload.
   This two-node architecture is common in production inspection lines.

Summary
--------

For a **Qt-based SHR 10GigE trigger and acquisition application** where data
throughput is the priority, the V3000 is the correct choice. The R8000's
superior GPU, AI, and display capabilities are irrelevant to this workload,
and its lack of native 10 GbE is a fundamental architectural mismatch for
an SHR camera that streams 1.25 GB/s continuously.
