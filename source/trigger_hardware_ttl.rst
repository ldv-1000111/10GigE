Option 2 — TTL-24V Hardware Trigger (Line1)
=============================================

The SHR camera's **Industrial TTL-24V I/O interface** accepts a direct
electrical trigger signal on its input line (``Line1``). This is the
lowest-latency and highest-precision trigger method — the signal path is
entirely in hardware with no software or network stack involved.

The signal goes directly to the **SHR camera's I/O connector**. The
Bedrock V3000 plays no role in the trigger timing — it simply receives
the frame that results.

Signal Specifications
----------------------

The SVS-Vistek industrial I/O interface is specified as TTL-24V, with both
electrical and optical inputs.

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Parameter
     - Specification
   * - **Input type**
     - Opto-isolated (electrical input)
   * - **Accepted voltage range**
     - 5V to 24V (opto-isolated input accepts this wide range)
   * - **Logic HIGH**
     - ≥ 5V (signal active)
   * - **Logic LOW**
     - ≤ 1V (signal inactive)
   * - **Signal polarity**
     - Rising edge (default) or Falling edge — configurable
   * - **Minimum pulse width**
     - ~1 µs (check camera manual for exact value)
   * - **Maximum trigger rate**
     - Limited by sensor readout time (exposure + readout)
   * - **Isolation**
     - Opto-isolated — no ground loop between source and camera

.. note::
   Because the input is **opto-isolated**, the external computer and the
   SHR camera do not need to share a common ground. This is essential for
   industrial installations where ground loops are a significant source of
   noise. The external computer can operate at 3.3V, 5V, 12V, or 24V —
   as long as the voltage on the camera's input pin is within range, it
   will trigger.

Compatible Signal Sources
--------------------------

.. list-table::
   :header-rows: 1
   :widths: 30 25 45

   * - Source
     - Voltage
     - Notes
   * - Raspberry Pi GPIO
     - 3.3V
     - Below the 5V minimum — use a level shifter or transistor driver
   * - Arduino GPIO
     - 5V
     - Connect directly with a current-limiting resistor (~330Ω)
   * - Industrial PC GPIO card
     - 3.3V–5V
     - May need a transistor driver for reliable opto-isolation drive
   * - PLC digital output (NPN)
     - 12V or 24V
     - Connect directly — PLC output drives the opto-isolator LED
   * - PLC digital output (PNP)
     - 12V or 24V
     - Connect with appropriate polarity — check camera I/O guide
   * - Bedrock V3000 GPIO
     - 3.3V
     - Use a transistor or level shifter to drive 5V+ into the camera
   * - FPGA / microcontroller
     - 3.3V or 5V
     - Standard digital output with current limiting resistor

Wiring Diagram
---------------

.. code-block:: text

   External Computer / PLC              SHR Camera I/O Connector
   ─────────────────────────            ─────────────────────────

   5V–24V Digital Output ──────R────►  IN+  (Line1 input)
                                        │
                         ┌──────────────┘  (opto-isolator inside camera)
                         │
   GND  ────────────────►  IN-  (Line1 ground)

   R = current-limiting resistor
     At  5V: 330 Ω  (gives ~10 mA through opto LED)
     At 12V: 820 Ω
     At 24V: 1.8 kΩ  (standard PLC output)

For the Bedrock V3000's 3.3V GPIO (if using V3000 as the trigger source
rather than an external computer), add a transistor driver:

.. code-block:: text

   V3000 GPIO (3.3V) ──────── Base (via 1kΩ) ──► NPN transistor (e.g. 2N2222)
   V3000 GND ──────────────── Emitter
   12V supply ─────── R ───── Collector ────────► IN+ on camera
   Camera IN- ─────────────── GND

.. warning::
   Always consult the **SVS-Vistek SHR I/O Quick Guide**
   (``QuickGuide SVCam I/O``, available on the camera's product page)
   for the exact pin assignments of your specific SHR model's I/O
   connector before wiring. Pin layouts vary between connector types.
   Incorrect wiring can damage the camera's I/O circuitry.

Vimba X Configuration for Hardware Trigger
--------------------------------------------

Configure the camera to use the hardware line as the trigger source.
This is set on the V3000 via Vimba X during camera setup:

.. code-block:: cpp

   // Configure hardware trigger on Line1
   setEnum(camera, "TriggerSelector",   "FrameStart");
   setEnum(camera, "TriggerSource",     "Line1");

   // Rising edge: camera fires when signal goes LOW → HIGH
   // FallingEdge: camera fires when signal goes HIGH → LOW
   setEnum(camera, "TriggerActivation", "RisingEdge");

   setEnum(camera, "TriggerMode",       "On");

   // Optional: trigger delay in microseconds
   // (useful for strobe synchronisation)
   // setFloat(camera, "TriggerDelay", 100.0);  // 100 µs delay

   runCmd(camera, "AcquisitionStart");

SafeTrigger — Protecting Against Invalid Triggers
---------------------------------------------------

The SHR camera's **SafeTrigger** feature (enabled in camera firmware)
adds protection against triggers arriving while the sensor is still reading
out the previous frame. Without SafeTrigger, a trigger arriving during
readout is silently ignored — causing a missed frame that is hard to detect.

With SafeTrigger enabled, the camera queues the trigger and fires it at
the earliest valid moment after the current readout completes:

.. code-block:: cpp

   // Enable SafeTrigger (if supported by your SHR firmware version)
   // Check availability in Vimba X Viewer → TriggerControl category
   setEnum(camera, "TriggerOverlap", "ReadOut");
   // "ReadOut" = accept trigger during readout, execute when ready
   // "Off"     = reject triggers during readout (default)

Strobe Output Synchronisation
-------------------------------

The SHR's integrated 4-channel LED strobe controller can be synchronised
to the hardware trigger so the flash fires at exactly the right moment.
Configure via the ``StrobeControl`` features in Vimba X:

.. code-block:: cpp

   // Enable strobe output on Line2 (check your camera's I/O map)
   setEnum(camera, "LineSelector",   "Line2");
   setEnum(camera, "LineMode",       "Output");
   setEnum(camera, "LineSource",     "ExposureActive");
   // LineSource = ExposureActive: output HIGH during sensor exposure

   // The strobe controller drives your LED driver directly from
   // the camera I/O — no additional timing logic needed

Measuring Trigger Latency
--------------------------

For precision applications, measure the actual hardware trigger latency
(time between rising edge on Line1 and first pixel being read out):

.. code-block:: cpp

   // Read the camera's frame timestamp from chunk data
   // Compare with a timestamp from your trigger hardware
   // Typical hardware trigger latency: 1–10 µs (camera firmware dependent)

   FeatureContainerPtr chunkData;
   pFrame->AccessChunkData(chunkData);
   FeaturePtr tsFeature;
   chunkData->GetFeatureByName("ChunkTimestamp", tsFeature);
   VmbInt64_t cameraTs = 0;
   tsFeature->GetValue(cameraTs);
   // cameraTs is in camera clock ticks — convert to µs using
   // GevTimestampTickFrequency feature

References
-----------

- SVS-Vistek SHR I/O Quick Guide (on camera product page):
  https://www.svs-vistek.com/en/industrial-cameras/svs-camera-detail.php?id=shr411MXGE
- SVS-Vistek SHR 10GigE Manual:
  https://www.alliedvision.com/fileadmin/content/documents/products/cameras/various/Manual_SHR_10GigE.pdf
