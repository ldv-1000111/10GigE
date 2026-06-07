GigE Vision: Continuous Streaming Explained
============================================

Understanding how GigE Vision works at the protocol level is essential for
appreciating why host platform selection matters so much for SHR cameras.

How GigE Vision Streaming Works
---------------------------------

GigE Vision is built on UDP/IP. Once ``AcquisitionStart`` is issued, the camera
begins transmitting image data packets at the maximum rate the interface allows.
The host does not request individual frames — it runs a **receive engine** that
continuously accepts incoming packets and assembles them into frame buffers.

.. code-block:: text

   Camera                          Host (Vimba X)
   ──────                          ──────────────
   AcquisitionStart ────────────►  StartCapture + QueueFrame(s)
                                          │
   [sensor exposes]                       │
   [pixel data → 10GigE]  ─────────────► NIC DMA → Frame buffer 1
   [next frame]           ─────────────► NIC DMA → Frame buffer 2
   [next frame]           ─────────────► NIC DMA → Frame buffer 3
        ↑                                      ↓
        └── continuous, always ──────  FrameReceived() callback
                                       → application processes frame
                                       → QueueFrame() to reuse buffer

The critical point: **the stream never pauses between triggers**. Even in
``SingleFrame`` or externally-triggered mode, the camera transmits the frame
data as soon as the sensor has finished exposing. The host receive engine must
be ready at all times.

What "Triggering" Actually Controls
-------------------------------------

When you set ``TriggerMode`` to ``On`` and ``TriggerSource`` to ``Line1`` or
``Software``, you are controlling **when the sensor exposes** — not when the
data flows on the wire. The timing sequence is:

.. code-block:: text

   Trigger signal received
        │
        ▼
   Sensor begins exposure (ExposureTime µs)
        │
        ▼
   Sensor readout begins (rolling shutter: top to bottom)
        │
        ▼
   Pixel data transmitted over 10GigE ──► Host NIC ──► DMA ──► Frame buffer
        │
        ▼
   FrameReceived() callback fires in Vimba X

The 10GigE wire is active during the readout and transmission phase. For a
151 MP frame at 12-bit packed, this transmission takes approximately:

.. code-block:: text

   160 MB ÷ 1.25 GB/s ≈ 128 ms per frame

During those 128 ms, the NIC is at full utilisation. The next trigger can
fire as soon as the current frame finishes transmitting (in continuous mode)
or be held off (in triggered mode), but the **NIC receive path is always armed**.

Buffer Management Under Continuous Streaming
---------------------------------------------

Vimba X manages a ring of frame buffers. The golden rule is that the
application must **requeue buffers faster than the camera fills them**.
If the application falls behind:

.. code-block:: text

   Camera sends frame N+1
        │
        ▼
   Vimba X has no free buffer ──► frame N+1 dropped
        │
        ▼
   GetReceiveStatus() returns VmbFrameStatusIncomplete or similar

This is not recoverable without stopping and restarting acquisition. The
solution is a sufficient number of buffers (5–8 for SHR) and a fast enough
host to process and requeue them before the next frame arrives.

At 6.1 fps (shr411), you have ~164 ms between frames to:

- Complete the ``FrameReceived()`` callback
- Transform the frame (debayer)
- Optionally save it
- Call ``QueueFrame()`` to reuse the buffer

The Bedrock V3000's CPU throughput and NVMe write speed are sized to meet
this budget comfortably even at 151 MP.

Implications for the Qt Application
--------------------------------------

In this architecture, the Qt application's role in the data path is
deliberately minimal:

- **Qt does not receive frames** — Vimba X callbacks handle this on a
  dedicated high-priority thread
- **Qt does not write to disk** — a separate worker thread handles NVMe writes
- **Qt triggers exposures** — a simple feature write: ``TriggerSoftware``
  ``RunCommand()`` or monitoring the hardware trigger line
- **Qt displays status** — frame count, frame rate, buffer health, errors

This clean separation means the Qt event loop never competes with the
acquisition pipeline. Even a modest CPU can run both without interference,
provided the threading architecture is correct (see :doc:`async_acquisition`).
