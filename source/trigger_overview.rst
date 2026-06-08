External Trigger — Overview & Selection
=========================================

The SHR 10GigE camera supports three external trigger mechanisms. Understanding
which to use depends on who is sending the trigger, what timing precision is
required, and whether additional wiring is acceptable.

How Triggering Fits the Streaming Architecture
------------------------------------------------

As covered in :doc:`platform_streaming`, the SHR camera streams data
continuously once ``AcquisitionStart`` is issued. A trigger does **not**
start or stop the data stream — it controls **when the sensor exposes**.

.. code-block:: text

   Continuous data stream:  ────────────────────────────────────────────►
                                    ↑           ↑           ↑
                              Trigger #1   Trigger #2   Trigger #3
                              (exposure)   (exposure)   (exposure)
                                  │             │             │
                              Frame in      Frame in      Frame in
                              stream        stream        stream

Between triggers the camera transmits no image data (in triggered mode).
The Vimba X receive engine stays armed and ready at all times.

The Three Trigger Methods
--------------------------

.. list-table::
   :header-rows: 1
   :widths: 5 25 20 20 30

   * - #
     - Method
     - Signal type
     - Precision
     - Best for
   * - 1
     - :doc:`trigger_action_commands`
       (Trigger over Ethernet)
     - UDP broadcast packet
       on the 10GigE network
     - ~microseconds
     - External PC on same network;
       multi-camera sync; no wiring
   * - 2
     - :doc:`trigger_hardware_ttl`
       (TTL-24V Line1)
     - 5V–24V digital pulse
       on camera I/O connector
     - ~nanoseconds
     - PLC, microcontroller, or
       hardware requiring hard real-time
   * - 3
     - :doc:`trigger_software_qt`
       (Software via Qt App)
     - Network message → V3000 →
       Vimba X TriggerSoftware
     - ~1–10 ms
     - Simplest integration;
       latency not critical

Decision Guide
---------------

.. code-block:: text

   External computer sending the trigger?
           │
           ├─ Is it a PC / computer on the same Ethernet network?
           │          └─► Option 1: Action Commands (UDP broadcast)
           │                        No wiring. Microsecond precision.
           │
           ├─ Is it a PLC, microcontroller, or hardware timer?
           │          └─► Option 2: TTL-24V hardware line
           │                        Wire to camera I/O. Nanosecond precision.
           │
           └─ Simplest possible integration, latency not critical?
                      └─► Option 3: Send message to V3000 Qt app
                                    App calls TriggerSoftware. ~ms latency.

.. note::
   **Options 1 and 2 are not mutually exclusive.** You can configure the
   camera to accept Action Commands (Ethernet trigger) from one source AND
   a hardware TTL pulse from another simultaneously using the camera's
   built-in PLC (programmable logic) and trigger sequencer features.

Common Vimba X Trigger Setup (All Methods)
--------------------------------------------

Regardless of which method is used, the following base configuration is
required in Vimba X before any external trigger will fire the sensor:

.. code-block:: cpp

   // 1. Select which event will trigger frame acquisition
   setEnum(camera, "TriggerSelector", "FrameStart");

   // 2. Set the source — different per method (see each chapter)
   setEnum(camera, "TriggerSource", "Line1");      // hardware
   // or "Action1"                                 // Action Commands
   // or "Software"                                // software trigger

   // 3. Set the active edge — rising edge fires on 0→1 transition
   setEnum(camera, "TriggerActivation", "RisingEdge");

   // 4. Enable trigger mode
   setEnum(camera, "TriggerMode", "On");

   // 5. Start acquisition — camera now waits for triggers
   runCmd(camera, "AcquisitionStart");

With ``TriggerMode`` set to ``On``, the camera will not expose until a
trigger arrives on the configured source. The stream pipeline on the V3000
side is already running via ``StartCapture()`` and queued buffers.
