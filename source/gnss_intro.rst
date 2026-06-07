GNSS/NMEA Metadata Injection — Overview
=========================================

This part covers the design and implementation of **GNSS (GPS/GNSS) position
injection into SHR image metadata**, linking every captured frame to the
geographic coordinates of the platform at the moment of exposure.

This is essential for aerial imaging, mobile inspection, and any application
where the physical location of the camera during capture must be traceable.

The GNSS receiver connects to the **Bedrock V3000 via RS232**, which the SHR
camera itself also exposes on its I/O connector — giving two independent paths
for serial data, both usable in this architecture.

What We Are Building
---------------------

.. code-block:: text

   GNSS Receiver
   (RS232, NMEA 0183)
          │
          │  RS232 → USB adapter or V3000 native serial
          │
   Bedrock V3000
   ├── Qt Serial Reader Thread
   │     └── parses NMEA sentences → GnssRecord (lat, lon, alt, time, fix)
   │
   ├── Vimba X Acquisition Thread
   │     └── on FrameReceived: timestamps frame → looks up latest GnssRecord
   │                           → writes per-frame JSON sidecar file
   │
   └── Qt Main Window
         └── live GNSS status panel
             frame geo-tag indicator
             coordinate display

Metadata Strategy — Why Not Camera Chunk Data?
-----------------------------------------------

The natural first question is: can GNSS data be injected directly into the
GigE Vision **chunk data** payload — i.e., appended to the frame at the camera
firmware level?

**The answer for SHR cameras is: not directly.** Here is why:

GigE Vision chunk data is generated **inside the camera** and appended to the
image payload before it leaves the sensor. The available chunk fields (exposure
time, gain, frame counter, timestamp, pixel format, etc.) are defined by the
camera firmware and exposed as ``Chunk*`` GenICam features. There is no standard
GigE Vision mechanism for the host to inject arbitrary external data — such as
a GNSS fix from a serial receiver — into the camera's outgoing chunk stream.

The SHR cameras do support reading chunk data via Vimba X:

.. code-block:: cpp

   // Read camera-generated chunk data in FrameReceived()
   FeatureContainerPtr chunkData;
   pFrame->AccessChunkData(chunkData);

   FeaturePtr exposureChunk;
   chunkData->GetFeatureByName("ChunkExposureTime", exposureChunk);
   double expTime = 0.0;
   exposureChunk->GetValue(expTime);

But the chunk fields are read-only from the host perspective — the camera
firmware writes them, not your application.

**The correct architecture is a host-side sidecar approach** — the application
correlates each frame with the current GNSS fix using a precise timestamp and
writes the geolocation data alongside the image file.

.. note::
   Some camera models support a ``ChunkUserDefinedValue`` or similar custom
   chunk field. If a future SHR firmware update or a custom SVS-Vistek
   variant supports this, the GNSS string could be written to a camera
   register before each triggered frame, and it would appear in the chunk
   data of the resulting image. This would be the cleanest solution but
   requires camera firmware support that is not currently documented for
   the SHR series. The sidecar approach described here is universal and
   works with any camera and any GigE Vision SDK version.

The Sidecar Metadata Approach
-------------------------------

For each captured frame, the application writes a companion **JSON sidecar
file** containing:

.. code-block:: json

   {
     "frame_index": 42,
     "frame_filename": "frame_00042.ppm",
     "capture_timestamp_utc": "2026-06-07T14:23:11.847Z",
     "camera_timestamp_ns": 18374628374,
     "gnss": {
       "fix_type": "3D",
       "fix_quality": 1,
       "latitude_deg":  48.137154,
       "longitude_deg": 11.576124,
       "altitude_m":    524.3,
       "speed_knots":   0.0,
       "track_deg":     0.0,
       "hdop":          0.9,
       "satellites":    9,
       "nmea_utc_time": "142311.847",
       "nmea_sentence": "$GNGGA,142311.847,4808.2292,N,01134.5674,E,1,09,0.9,524.3,M,..."
     }
   }

This sidecar travels with the image and can be consumed by any GIS software,
photogrammetry pipeline, or custom analysis tool.

RS232 on the Bedrock V3000
---------------------------

The Bedrock V3000 exposes serial via a **USB console port** (mini-USB) and
optionally via a native UART header. For a clean industrial GNSS connection,
a standard **USB-to-RS232 adapter** (CP2102, FTDI FT232, or PL2303 chipset)
plugged into one of the V3000's USB 3.2 ports presents as ``/dev/ttyUSB0``
or ``/dev/ttyUSB1`` on Linux.

The SHR camera itself also has an **RS232 port** on its TTL-24V I/O connector —
this is intended for camera control, not for connecting a GNSS receiver.
The GNSS receiver connects to the **host (V3000)**, not the camera.

.. note::
   Many GNSS receivers output NMEA at 4800 or 9600 baud by default.
   High-precision RTK receivers (e.g. u-blox F9P) can output at 115200 baud
   with 10 Hz or higher update rates. For aerial imaging applications,
   a 10 Hz receiver significantly improves position accuracy between frames.
