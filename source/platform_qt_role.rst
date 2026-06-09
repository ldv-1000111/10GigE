Qt's Role in the Acquisition Architecture
==========================================

The application is split across two processes: a **headless C++ backend**
on the V3000 that owns the entire data pipeline, and a **Qt QML frontend**
on the Android tablet that owns the entire user interface. Qt plays a
different role in each process.

On the **V3000**, Qt provides ``QCoreApplication``, ``QTcpServer``,
``QTimer``, and ``QThread`` — infrastructure only. There is no UI.

On the **Android tablet**, Qt provides the full QML touch UI — camera
previews, trigger controls, GNSS display, collapsible settings panels.
There is no Vimba X.

Understanding this separation is essential for designing an application
that stays responsive while Vimba X saturates the 10 GbE link.

Architecture Overview
----------------------

The application consists of three independent layers, each running on
separate threads:

.. code-block:: text

   ┌─────────────────────────────────────────────────────┐
   │                  Qt Application Layer                │
   │  ┌─────────────────┐   ┌──────────────────────────┐ │
   │  │  Main UI Thread  │   │   Qt Signals / Slots     │ │
   │  │  - Trigger btn   │   │   - frameReceived(index) │ │
   │  │  - Status panel  │◄──│   - statsUpdated()       │ │
   │  │  - Preview thumb │   │   - errorOccurred()      │ │
   │  └────────┬─────────┘   └──────────────────────────┘ │
   └───────────┼─────────────────────────────────────────┘
               │ QMetaObject::invokeMethod (thread-safe)
   ┌───────────▼─────────────────────────────────────────┐
   │             Vimba X Acquisition Layer                │
   │  ┌──────────────────────────────────────────────┐   │
   │  │  FrameObserver (Vimba X callback thread)     │   │
   │  │  - Validates frame status                    │   │
   │  │  - Copies frame metadata                     │   │
   │  │  - Signals worker thread                     │   │
   │  │  - QueueFrame() immediately                  │   │
   │  └──────────────────────────────────────────────┘   │
   └─────────────────────────────────────────────────────┘
               │ std::condition_variable notify
   ┌───────────▼─────────────────────────────────────────┐
   │               Frame Processing Layer                 │
   │  ┌──────────────────────────────────────────────┐   │
   │  │  Worker Thread                               │   │
   │  │  - Debayer (VmbImageTransform)               │   │
   │  │  - Write to NVMe                             │   │
   │  │  - Generate thumbnail for Qt preview         │   │
   │  └──────────────────────────────────────────────┘   │
   └─────────────────────────────────────────────────────┘

What Qt Controls
-----------------

**Trigger commands**

   For software triggering, Qt issues a single Vimba X feature write —
   a lightweight operation that completes in microseconds:

   .. code-block:: cpp

      // In a Qt slot connected to a button:
      void MainWindow::onTriggerClicked()
      {
          FeaturePtr feature;
          m_camera->GetFeatureByName("TriggerSoftware", feature);
          feature->RunCommand();    // non-blocking, microseconds
      }

   For hardware triggering (TTL line), Qt simply enables or disables
   the trigger mode — the actual timing is handled by the external signal.

**Acquisition start/stop**

   Qt buttons map to ``AcquisitionStart`` / ``AcquisitionStop`` feature
   commands. These are called once per session, not per frame.

**Parameter control**

   Exposure time, gain, pixel format, and ROI are set via Qt spinboxes
   and combo boxes, each writing a single Vimba X feature. These are
   infrequent operations with no throughput impact.

**Status monitoring**

   Frame count, frame rate, buffer queue depth, and error counts are
   updated via Qt signals from the acquisition layer. The UI polls or
   receives these at 1–10 Hz — imperceptible overhead.

**Preview display**

   If a live preview is shown in the Qt window, it must be a **downsampled
   thumbnail**, not the full frame. Displaying a 151 MP image in a Qt widget
   would require gigabytes of GPU texture upload per frame. A 1920×1080
   downsampled thumbnail generated in the worker thread and uploaded as a
   ``QImage`` is the correct approach.

What Qt Does NOT Do
--------------------

The following must never happen on the Qt main thread or inside a Vimba X
callback:

- Debayering full-resolution frames
- Writing frames to NVMe
- Copying large frame buffers
- Any blocking operation > ~5 ms

If any of these run on the Vimba X callback thread, ``QueueFrame()`` is
delayed and the next incoming frame may find no free buffer — causing drops.
If they run on the Qt main thread, the UI freezes and trigger buttons become
unresponsive.

Thread Layout on the V3000
---------------------------

In the two-process architecture, the V3000 runs **no Qt UI** — the
``QCoreApplication`` backend daemon uses all 8 cores exclusively for
acquisition, processing, and the TCP server. There is no Qt rendering
competing for CPU time.

.. code-block:: text

   V3000 — SHR_Backend (QCoreApplication, headless)
   ──────────────────────────────────────────────────
   Core 0–1:  Vimba X Camera 1 acquisition (SCHED_FIFO, real-time)
   Core 2–3:  Vimba X Camera 2 acquisition (SCHED_FIFO, real-time)
   Core 4–5:  Frame worker + NVMe write threads
   Core 6:    GNSS NmeaReader + BackendServer TCP
   Core 7:    OS + systemd + idle

   Android tablet — SHR_Camera_Android (QGuiApplication)
   ───────────────────────────────────────────────────────
   All cores:  Qt Quick scene graph + BackendClient TCP
               (standard Android scheduler, no real-time needed)

This is the key advantage of the two-process split: the V3000 CPU is
100% dedicated to the acquisition pipeline. Qt rendering — which can
cause millisecond-scale jitter — runs on a completely separate device.

Core pinning on the V3000 is configured via the real-time scheduling
settings in :doc:`system_settings` and the ``LimitRTPRIO=99`` directive
in the ``shr-backend.service`` systemd unit (see :doc:`full_application`).
