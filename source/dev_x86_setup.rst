Development on a Generic x86 Machine
======================================

The complete software stack — Qt application, Vimba X SDK, GNSS reader,
sidecar writer, and trigger server — can be developed and tested on any
standard x86 Ubuntu 24.04 machine without a Bedrock V3000 or a physical
SHR camera. The **Vimba X Camera Simulator** provides synthetic frames
through the same API as a real camera, so the entire pipeline runs
identically.

What Works Without V3000 Hardware
-----------------------------------

.. list-table::
   :header-rows: 1
   :widths: 45 15 40

   * - Component
     - Works?
     - Notes
   * - Vimba X SDK — full C++ API
     - ✓
     - Camera Simulator provides synthetic frames
   * - Qt 6.8 application — UI, signals, slots
     - ✓
     - No difference from V3000
   * - Image Transform Library — debayer, colour
     - ✓
     - Runs on CPU, no special hardware needed
   * - GNSS reader — QSerialPort, NMEA parser
     - ✓
     - Use ``socat`` virtual serial port (see :doc:`gnss_full_implementation`)
   * - JSON sidecar writer
     - ✓
     - No difference
   * - TriggerServer — UDP listener
     - ✓
     - Loopback interface works fine
   * - Action Commands — UDP sender
     - ✓
     - Send to loopback or LAN
   * - Real SHR camera at full resolution
     - ✗
     - Requires 10GbE NIC + physical camera
   * - 1.25 GB/s sustained throughput testing
     - ✗
     - Requires V3000 native NIC
   * - ECC memory protection
     - ✗
     - Consumer hardware typically lacks ECC

Differences vs the V3000 in Development
-----------------------------------------

**10GigE NIC**
   The only hardware gap that matters for camera work. A generic machine
   has 1GbE onboard at best. For development with the Camera Simulator,
   no NIC is involved at all. For testing with a real SHR camera, a PCIe
   10GbE add-in card (e.g. Intel X550-T1, Mellanox ConnectX-3) works
   perfectly — just not at sustained 1.25 GB/s production rates.

**Real-time scheduling**
   Works on any Ubuntu 24.04 machine but a desktop install has more
   background processes than the lean V3000 deployment image. Add
   ``isolcpus`` only on the V3000 — not needed for development.

**Everything else**
   Qt, Vimba X, CMake, the C++ code, the serial port, the sidecar
   files — all identical. Code compiled on a dev machine runs unchanged
   on the V3000.

The Vimba X Camera Simulator
------------------------------

The Camera Simulator enables you to use Vimba X without a physical camera.
It is automatically installed with Vimba X. It appears as a real
GenICam camera in ``VmbSystem::GetCameras()`` and produces synthetic
test images at configurable resolution and frame rate.

Available simulated camera models are listed in
``VimbaCameraSimulatorTLPresets.json`` in the Vimba X ``cti/`` directory.
You can add more simulated camera models to this file. Color models will
only appear in the camera list if width is divisible by 2 and height is
divisible by 8.

.. note::
   The timing of simulated cameras doesn't exactly match the timing
   of real camera hardware, and available camera features do not represent
   the full feature set of the actual cameras. Functionally however,
   the acquisition pipeline, frame callbacks, image transform, and all
   application logic run correctly.

Setting Up the Camera Simulator
---------------------------------

The Simulator TL is installed automatically with Vimba X. Verify it is
active:

.. code-block:: bash

   ls ~/VimbaX_2026-1/cti/VimbaCameraSimulatorTL.cti
   # Should exist — no separate install needed

Verify it appears in Vimba X Viewer:

.. code-block:: bash

   ~/VimbaX_2026-1/bin/VimbaXViewer

A simulated camera named ``DEV_SimulatedCamera_...`` should appear in
the camera list. If it does not, check that the Camera Simulator TL
is not disabled in ``VmbC.xml``.

Configuring Simulated Camera Resolution
-----------------------------------------

To simulate SHR-like resolutions, edit
``VimbaCameraSimulatorTLPresets.json`` in the ``cti/`` directory and add
a custom preset:

.. code-block:: json

   {
     "cameras": [
       {
         "name":        "SHR_sim_101MP",
         "width":       11648,
         "height":      8742,
         "pixel_format":"BayerGR12",
         "frame_rate":  8.7
       },
       {
         "name":        "SHR_sim_151MP",
         "width":       14192,
         "height":      10640,
         "pixel_format":"BayerGR12",
         "frame_rate":  6.1
       }
     ]
   }

.. warning::
   Width must be divisible by 2 and height by 8 for colour (Bayer)
   formats. The values above already satisfy this. After editing,
   restart Vimba X Viewer or your application to pick up the changes.

.. warning::
   With Bayer pixel formats, the moving test image from the Camera
   Simulator causes wrong colours during colour interpolation.
   This is a known simulator limitation — the debayer output will have
   colour artefacts on the synthetic test image. The debayer code itself
   is correct; use a static test image or single-colour frame to verify
   colour accuracy.

Customising the Simulated Image
---------------------------------

Use ``VimbaCameraSimulatorTL.xml`` in the ``cti/`` directory to
customise simulated images. Follow the instructions in the comments to
use your own image or image series.

To use a real captured raw frame as the simulator input — for example
a previously captured SHR Bayer frame saved as a ``.bin`` file:

.. code-block:: xml

   <!-- In VimbaCameraSimulatorTL.xml -->
   <ImageSource>
       <Type>File</Type>
       <Path>/home/luis/test_frames/shr411_bayer12.bin</Path>
   </ImageSource>

.. note::
   Vimba X uninstall deletes ``VimbaCameraSimulatorTL.xml``. Save a
   copy outside the Vimba X install directory and restore it after
   updates.

Connecting to the Simulated Camera in Code
-------------------------------------------

No code changes are needed. ``VmbSystem::GetCameras()`` returns the
simulated camera alongside any real cameras. The application connects
by index (first found) or by the simulated camera's ID:

.. code-block:: cpp

   // Works unchanged — Camera Simulator appears as a real camera
   CameraPtrVector cameras;
   system.GetCameras(cameras);

   // cameras[0] may be the simulator if no real camera is connected
   CameraPtr camera = cameras.at(0);
   camera->Open(VmbAccessModeFull);

   // All feature access works normally
   setEnum(camera, "PixelFormat", "BayerGR12");
   setInt (camera, "Width",       11648);
   setInt (camera, "Height",      8742);

   // Acquisition pipeline identical — FrameReceived() fires with
   // synthetic frames at the configured frame rate

.. tip::
   To force the application to use only the simulator and never a real
   camera, load only the Camera Simulator TL at startup:

   .. code-block:: cpp

      // Load only the simulator TL — ignores GigE and USB TLs
      system.Startup(
          "/path/to/VimbaX/cti/VimbaCameraSimulatorTL.cti");

What to Test on the Dev Machine
---------------------------------

With the Camera Simulator running, you can fully exercise:

- The complete async acquisition pipeline (``StartCapture``,
  ``QueueFrame``, ``FrameReceived``, ``QueueFrame`` cycle)
- The Image Transform pipeline (Bayer → RGB8, debayer modes,
  colour correction matrix)
- The GNSS tagging path (with ``socat`` virtual serial port)
- The JSON sidecar writer and output file structure
- The Qt UI — trigger button, status panel, GNSS panel, preview
- All three trigger methods:
   - Software trigger: ``TriggerSoftware RunCommand()`` works on the simulator
   - Action Commands: simulator responds to ``Action1`` if configured
   - Hardware TTL: not applicable without physical camera

What Requires Real Hardware
-----------------------------

- Sustained 10GbE throughput testing
- Full SHR resolution (101–151 MP) at production frame rates
- Hardware trigger timing precision (nanoseconds)
- SafeTrigger, strobe controller, TTL I/O
- ECC memory protection validation

Development Workflow
---------------------

The recommended path from dev machine to V3000 production:

.. code-block:: text

   1. Develop & test on dev machine
      └── Camera Simulator + socat GNSS + loopback triggers
               │
               ▼
   2. Test with real camera on dev machine
      └── Add PCIe 10GbE NIC + SHR camera
          Verify pipeline at reduced frame rate (1GbE limit)
               │
               ▼
   3. Deploy to Bedrock V3000
      └── Copy binary or rebuild — zero code changes needed
          Apply OS tuning (MTU, buffers, real-time scheduling)
          Run at full 1.25 GB/s production throughput

Deploying to the V3000
-----------------------

Since the dev machine and V3000 are both x86_64 Ubuntu 24.04, deploy
by simply copying the compiled binary and Vimba X shared libraries:

.. code-block:: bash

   # On the dev machine — build release binary
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
         -DVIMBAX_DIR=~/VimbaX_2026-1
   cmake --build build -j$(nproc)

   # Copy to V3000 (replace v3000-ip with actual IP)
   scp build/SHR_Camera_App luis@v3000-ip:~/
   scp build/libVmbCPP.so build/libVmbImageTransform.so luis@v3000-ip:~/

   # On the V3000 — run
   ssh luis@v3000-ip
   export LD_LIBRARY_PATH=~/VimbaX_2026-1/api/lib/x86_64:$LD_LIBRARY_PATH
   ./SHR_Camera_App 192.168.10.42 0

Alternatively, since the V3000 runs the same OS, install Vimba X on it
directly and rebuild from source — the CMake build is identical.
