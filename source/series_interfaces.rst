Interface Options
==================

SHR cameras are available with two interface technologies: **10GigE Vision**
and **CoaXPress**. The right choice depends on resolution, required frame rate,
infrastructure, and whether a frame grabber is acceptable.

10GigE Vision
--------------

10GigE (10 Gigabit Ethernet) is the interface used on the SHR 10GigE models
covered throughout this tutorial.

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Property
     - Value / Notes
   * - Bandwidth
     - Up to **1.25 GB/s** (10 Gbps)
   * - Cable type
     - Cat 6 or higher Ethernet (up to 100 m)
   * - Frame grabber required
     - **No** — connects to any 10GigE NIC
   * - Protocol
     - GigE Vision (GenICam compliant)
   * - PTP sync
     - Yes — IEEE 1588 Precision Time Protocol
   * - Action Commands
     - Yes — Trigger over Ethernet (broadcast to multiple cameras)
   * - IP address
     - Assigned via DHCP or static
   * - SHR models
     - shr461, shr661, shr411 (and -T variants)
   * - Max resolution over 10GigE
     - 151 MP (shr411) at 6.1 fps

**Advantages:**

- No specialist hardware — any server or workstation with a 10GigE NIC works
- Long cable runs (up to 100 m) without repeaters
- Easy multi-camera setups on a switch
- Simpler integration into IT infrastructure
- Lower total system cost

**Limitations:**

- 1.25 GB/s bandwidth cap limits frame rate at high resolution
- Not suitable for 245 MP at practical frame rates

CoaXPress (CXP-6 and CXP-12)
------------------------------

CoaXPress uses coaxial cables with a proprietary high-speed protocol.
It requires a **PCIe frame grabber** in the host PC.

.. list-table::
   :header-rows: 1
   :widths: 35 30 35

   * - Property
     - CXP-6
     - CXP-12
   * - Per-channel bandwidth
     - 6.25 Gbps
     - 12.5 Gbps
   * - Cable type
     - 75 Ω coaxial (up to 40 m)
     - 75 Ω coaxial
   * - Frame grabber required
     - **Yes**
     - **Yes**
   * - SHR models
     - shr461CCX, shr411CCX (and M variants)
     - shr661CCX12, shr811CCX12 (and M variants)

**Advantages:**

- Higher bandwidth per link than 10GigE
- Deterministic latency (no network stack)
- Better suited for highest frame rates

**Limitations:**

- Requires PCIe frame grabber (additional cost and PCIe slot)
- Shorter cable runs (typically 40 m max)
- Less flexible multi-camera networking

Interface Selection Guide
--------------------------

.. list-table::
   :header-rows: 1
   :widths: 50 25 25

   * - Use case
     - 10GigE
     - CoaXPress
   * - Standard PC / workstation (no PCIe slot spare)
     - ✓
     -
   * - Long cable runs (> 40 m)
     - ✓
     -
   * - Multi-camera on a network switch
     - ✓
     -
   * - 245 MP at 12+ fps
     -
     - ✓
   * - 127 MP at 20+ fps
     -
     - ✓ (CXP-12)
   * - Lowest possible latency
     -
     - ✓
   * - Linux tutorial (this guide)
     - ✓
     -

.. note::
   This tutorial focuses exclusively on **10GigE** models. The Vimba X
   application code is identical for CoaXPress — only the transport layer
   installation and camera connection differ (see :doc:`series_models_245mp`
   for CXP-specific notes).
