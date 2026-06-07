Choosing the Right Host Platform
==================================

The SHR camera series is only half the system. The host PC that receives,
processes, and stores the image data is equally critical — and the wrong
choice here will bottleneck the entire pipeline regardless of camera quality.

This section explains why the `SolidRun Bedrock V3000
<https://www.solid-run.com/industrial-computers/bedrock-v3000-basic/>`_
is the recommended host platform for SHR 10GigE applications, and how it
maps to the specific demands of continuous GigE Vision streaming.

The Challenge: Sustained High-Bandwidth Streaming
---------------------------------------------------

A common misconception about GigE Vision triggering is that the host only
receives data when a frame is triggered. In reality, **GigE Vision cameras
stream continuously** once acquisition starts. The camera sends a constant
flow of image data at full interface bandwidth — the host must receive every
frame whether it needs it or not.

For the SHR 10GigE series this means:

.. code-block:: text

   shr461 (101 MP, 8.7 fps):   ~1.12 GB/s  — continuous, indefinitely
   shr661 (127 MP, 8.2 fps):   ~0.98 GB/s  — continuous, indefinitely
   shr411 (151 MP, 6.1 fps):   ~1.10 GB/s  — continuous, indefinitely

This is not a burst workload. It is a **sustained, uninterrupted data stream**
that the host NIC, DMA engine, memory bus, and storage subsystem must handle
without dropping a single packet — for hours or days at a time.

What This Means for Platform Selection
----------------------------------------

A host platform for SHR cameras must satisfy several simultaneous requirements
that compete for the same hardware resources:

**Network throughput**
   The NIC must sustain 10 Gbps receive continuously. Any NIC that cannot
   keep pace will cause ``VmbFrameStatusIncomplete`` errors across all frames
   — not just the ones that matter to the application.

**DMA and memory bandwidth**
   Every received frame is DMA'd directly into system RAM by the NIC. At
   1.25 GB/s, the memory bus must absorb this while simultaneously serving
   the CPU (Vimba X callbacks, frame processing, Qt rendering).

**Storage throughput**
   Frames selected for saving must be written to NVMe at rates that don't
   block the acquisition pipeline. Three PCIe Gen4 NVMe slots allow frame
   data to be striped or written in parallel without saturating a single
   device.

**Memory capacity**
   At 151 MP and 5 frame buffers, the Vimba X buffer pool alone is ~1.5 GB.
   Add the Qt application, OS, and any processing pipeline and 32–64 GB RAM
   is a practical minimum.

**Reliability**
   Continuous high-bandwidth DMA into system memory under sustained load is
   exactly the workload where DRAM bit errors cause the most damage —
   corrupting frame data in ways that are hard to detect. ECC RAM is not
   optional for production SHR deployments.

The Bedrock V3000 satisfies all of these requirements in a compact,
fanless, industrially-rated enclosure. The following sections explain each
advantage in detail.
