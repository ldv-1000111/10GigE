Application Architecture Overview
==================================

The complete application is split across two processes running on two
devices, connected by a wired USB-C link. This separation gives each
device exactly the responsibilities it is best suited for.

.. code-block:: text

   ┌─────────────────────────────────────────────────────────────────┐
   │  Bedrock V3000 — Ubuntu 24.04                                   │
   │                                                                 │
   │  ┌──────────────────────────────────────────────────────────┐   │
   │  │  SHR Acquisition Backend  (C++ / headless)               │   │
   │  │                                                          │   │
   │  │  CameraWorker × 2   ──►  Vimba X C++ API                 │   │
   │  │  FrameObserver      ──►  Image Transform Library         │   │
   │  │  NmeaReader         ──►  /dev/ttyUSB0 (GNSS RS232)       │   │
   │  │  SidecarWriter      ──►  /mnt/frames/*.json              │   │
   │  │  TriggerServer      ──►  UDP :9001 (Action Commands)     │   │
   │  │  BackendServer      ──►  TCP :9100 (Android client)      │   │
   │  └──────────────────────────────────────────────────────────┘   │
   │                              │                                  │
   └──────────────────────────────┼──────────────────────────────────┘
                                  │  USB-C cable
                                  │  USB NCM (network over USB)
                                  │  192.168.100.1 ◄──► 192.168.100.2
                                  │
   ┌──────────────────────────────┼──────────────────────────────────┐
   │  Android Tablet              │                                  │
   │                              │                                  │
   │  ┌───────────────────────────▼──────────────────────────────┐   │
   │  │  SHR Camera Control  (Qt 6.8 / QML)                      │   │
   │  │                                                          │   │
   │  │  BackendClient  ◄──►  TCP :9100 (status + commands)      │   │
   │  │  MainView.qml   ──►  ForeFlight-style dark UI            │   │
   │  │  CameraPanel    ──►  Stacked previews + metrics          │   │
   │  │  TriggerBar     ──►  Trigger both / cam1 / cam2          │   │
   │  │  GnssPanel      ──►  Live coordinates + fix quality      │   │
   │  │  SectionWidget  ──►  Collapsible config sections         │   │
   │  └──────────────────────────────────────────────────────────┘   │
   └─────────────────────────────────────────────────────────────────┘

Why This Split
--------------

**The V3000 owns the data pipeline.** It is the only device with:

- Native dual 10GbE SFP+ to receive SHR camera streams at 1.25 GB/s each
- The Vimba X SDK and GigE Transport Layer
- Three NVMe PCIe Gen4 slots for frame storage
- The USB-to-RS232 adapter for the GNSS receiver
- ECC DDR5 for DMA integrity

None of this is accessible from an Android tablet. The V3000 must be the
acquisition engine.

**The Android tablet owns the interface.** It provides:

- A touch-native screen in a portable, ergonomic form factor
- A rugged, battery-powered display that can be held by the operator
- Qt Quick / QML — a touch-first UI framework with fluid animations
- A familiar device for operators who use ForeFlight on the same tablet

**The USB-C link is wired and deterministic.** USB NCM over USB-C gives:

- ~400 Mbps throughput (USB 2.0) or ~3 Gbps (USB 3.0)
- Sub-millisecond latency for command messages
- No WiFi dependency — works underground, in the air, in RF-noisy environments
- A single cable that also charges the tablet if using USB-C PD

Process Responsibilities
-------------------------

.. list-table::
   :header-rows: 1
   :widths: 50 50

   * - V3000 Backend (C++ / headless)
     - Android App (Qt QML)
   * - Vimba X camera management
     - All UI rendering
   * - Async frame acquisition
     - Touch controls
   * - Image transformation (debayer)
     - Trigger buttons
   * - GNSS serial reader
     - Status displays
   * - JSON sidecar writer
     - Settings panels
   * - Frame storage to NVMe
     - Frame log display
   * - TriggerServer UDP :9001
     - Thumbnail preview
   * - BackendServer TCP :9100
     - BackendClient TCP connection
   * - Action Command handling
     - Command dispatch (trigger, start, stop, set)

The TCP Protocol
-----------------

The V3000 backend and Android app communicate over a simple
**JSON-over-TCP** protocol on port 9100. The V3000 pushes status
at ~10 Hz; the Android app sends commands on user interaction.

**V3000 → Android (status, ~10 Hz):**

.. code-block:: json

   {
     "type": "status",
     "cam1": {
       "frames": 1284, "fps": 6.1,
       "written_gb": 538.2, "bw_gbs": 1.19,
       "dropped": 0, "buf_free": 4,
       "geo_tagged": true
     },
     "cam2": {
       "frames": 1283, "fps": 6.1,
       "written_gb": 537.1, "bw_gbs": 1.18,
       "dropped": 0, "buf_free": 5,
       "geo_tagged": true
     },
     "gnss": {
       "valid": true, "fix": "GPS 3D",
       "lat": 48.1372, "lon": 11.5761,
       "alt_m": 524.3, "sats": 9, "hdop": 0.9,
       "utc": "14:23:11.847"
     },
     "sync": {
       "delta_ms": 0.8,
       "total_bw_gbs": 2.37,
       "dropped_total": 0
     }
   }

**Android → V3000 (commands, on user action):**

.. code-block:: json

   { "type": "trigger",    "target": "both"          }
   { "type": "trigger",    "target": "cam1"          }
   { "type": "trigger",    "target": "cam2"          }
   { "type": "start"                                 }
   { "type": "stop"                                  }
   { "type": "set", "param": "exposure_us",
                    "value": 5000                    }
   { "type": "set", "param": "pixel_format",
                    "value": "BayerGR12"             }
   { "type": "set", "param": "output_dir",
                    "value": "/mnt/frames"           }

**V3000 → Android (frame log events, on each frame):**

.. code-block:: json

   {
     "type": "frame",
     "cam": 1,
     "index": 1284,
     "geo_tagged": true,
     "process_ms": 163,
     "ts": "14:23:11.861"
   }

Document Structure for This Part
----------------------------------

Part IV is divided into two sub-parts reflecting the two processes:

.. list-table::
   :header-rows: 1
   :widths: 15 85

   * - Sub-part
     - Covers
   * - **IV-A: V3000 Backend**
     - Camera discovery, feature configuration, async acquisition,
       image transformation, frame storage, trigger handling,
       GNSS injection, TCP backend server
   * - **IV-B: Android Frontend**
     - Qt for Android build setup, QML UI structure, BackendClient,
       QML components (CameraPanel, SectionWidget, TriggerBar,
       GnssPanel), stylesheet, build and deploy


Thumbnail Streaming — Phase 2
-------------------------------

The current TCP protocol carries only **metadata** (frame counts, fps,
GNSS, bandwidth). The ``CameraPanel.qml`` preview shows a placeholder.
Thumbnail streaming is a planned Phase 2 feature.

When implemented, the V3000 backend will:

1. Downsample the debayered RGB8 frame to 1920×1080 after processing
2. JPEG-encode it (``libturbojpeg``, ~200–400 KB per thumbnail)
3. Send it as a binary frame alongside the JSON status message

The protocol extension:

.. code-block:: text

   # Header message (JSON):
   {"type":"thumb","cam":1,"index":1284,"width":1920,"height":1080,"jpeg_bytes":287432}

   # Immediately followed by 287432 bytes of raw JPEG data on the TCP stream

On the Android side, ``CameraPanel.qml`` replaces the placeholder
``Rectangle`` with an ``Image`` element sourced from a
``QQuickImageProvider`` backed by the decoded JPEG.

At 10 fps and ~300 KB per thumbnail, this adds ~3 MB/s per camera
(~6 MB/s total) to the USB NCM link — well within USB 2.0 NCM capacity
of ~400 Mbps (50 MB/s).

.. note::
   Thumbnail streaming is explicitly **not** in scope for the current
   implementation. The acquisition pipeline runs correctly at full
   resolution and frame rate without it. Add it once the core pipeline
   is stable and tested on hardware.
