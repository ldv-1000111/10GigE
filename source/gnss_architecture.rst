GNSS Injection Architecture
============================

The GNSS injection system adds two components to the existing acquisition
architecture: a **serial reader** and a **frame tagger**. Both must integrate
with the existing Vimba X pipeline without introducing any latency into the
acquisition path.

Thread Architecture
--------------------

.. code-block:: text

   Thread 0: Qt Main Thread
   ├── MainWindow UI
   ├── Trigger button → AcquisitionStart/Stop, TriggerSoftware
   ├── GNSS status panel (updated via Qt signals)
   └── Serial port open/close (QSerialPort belongs to this thread)

   Thread 1: Vimba X Acquisition (SCHED_FIFO, high priority)
   ├── FrameReceived() callback
   │   ├── Validate frame status
   │   ├── Read camera chunk timestamp
   │   ├── Snapshot GnssRecord from atomic shared pointer  ← key step
   │   ├── Signal worker thread (frame data + gnss snapshot)
   │   └── QueueFrame() immediately ← must be fast
   └── (never blocks, never does I/O)

   Thread 2: Frame Worker (high priority)
   ├── Receives frame data + GnssRecord from queue
   ├── VmbImageTransform (debayer)
   ├── Write image file (.ppm)
   ├── Write JSON sidecar file  ← GNSS data written here
   └── Emit Qt signal for UI update

   Thread 3: GNSS Serial Reader (normal priority)
   ├── Runs in Qt event loop via QSerialPort::readyRead signal
   │   (but moved to a QThread for isolation from UI)
   ├── Reads NMEA sentences from /dev/ttyUSB0
   ├── Validates checksum
   ├── Parses GGA and RMC sentences
   └── Atomically updates shared GnssRecord

Shared State: GnssRecord
--------------------------

The bridge between the serial reader thread and the acquisition callback
is a single **atomically-swapped shared pointer**. The serial reader writes
a new record; the acquisition callback reads the current one — with no
mutex contention in the hot path:

.. code-block:: cpp

   // GnssRecord.h
   struct GnssRecord
   {
       double      latitude    = 0.0;
       double      longitude   = 0.0;
       double      altitude_m  = 0.0;
       double      speed_kn    = 0.0;
       double      track_deg   = 0.0;
       double      hdop        = 99.9;
       int         satellites  = 0;
       int         fix_quality = 0;        // 0=none, 1=GPS, 2=DGPS, 4=RTK
       QString     utc_time;               // HHMMSS.sss from GGA
       QString     utc_date;               // DDMMYY from RMC
       QString     raw_gga;                // full raw sentence for archiving
       QString     raw_rmc;
       QDateTime   host_timestamp;         // host wall clock at parse time
       bool        valid = false;
   };

   // Shared between serial reader and acquisition callback
   // std::atomic<shared_ptr> requires C++20; use atomic_store/load for C++17
   extern std::shared_ptr<GnssRecord> g_currentGnss;  // atomic_store/load

   // In FrameReceived() — zero-contention read:
   auto gnssSnapshot = std::atomic_load(&g_currentGnss);

   // In serial reader — after parsing a valid sentence:
   auto newRecord = std::make_shared<GnssRecord>(parsedRecord);
   std::atomic_store(&g_currentGnss, newRecord);

Timestamp Correlation
----------------------

The most critical aspect of GNSS injection is correctly correlating the
GNSS fix time with the camera frame.

.. code-block:: text

   Timeline:

   GNSS receiver       Host clock              Camera
   ─────────────       ──────────              ──────
   1 Hz / 10 Hz fix    system clock            PTP or free-running
        │                    │                 hardware timestamp
        │                    │                 (ns resolution)
        ▼                    ▼                       ▼
   NMEA sentence        host_timestamp         ChunkTimestamp
   received              (wall clock)          (camera counter)

Three timestamps are available for correlation:

**NMEA UTC time (from GGA/RMC)**
   The time of the GNSS fix according to the receiver's internal clock,
   synchronized to GPS/GNSS system time. This is the most accurate time
   reference but has ~1 second latency from fix to NMEA output at 1 Hz.

**Host wall clock (``QDateTime::currentDateTimeUtc()``)**
   The V3000's system clock at the moment the NMEA sentence is received
   and parsed. Accurate to ~1–10 ms with NTP, or ~1 µs with PTP. This
   is what we use to correlate with the frame capture timestamp.

**Camera hardware timestamp (``ChunkTimestamp`` from chunk data)**
   The camera's internal free-running counter in nanoseconds, latched
   at the moment of sensor exposure. This is the most precise per-frame
   timestamp but is in camera time, not UTC.

**Recommended approach for this implementation:**

Use the **host wall clock** for correlation:

1. Record ``host_timestamp`` when each NMEA sentence is parsed
2. Record ``QDateTime::currentDateTimeUtc()`` in ``FrameReceived()``
3. The GNSS fix used is the one whose ``host_timestamp`` is closest to
   the frame's capture time — in practice this is simply the latest fix
   since GNSS sentences arrive continuously and frames are the events

For aerial precision applications requiring sub-second accuracy, augment
with PTP synchronisation of the camera (GigE Vision supports IEEE 1588)
and correlate camera timestamps with GNSS UTC time directly.
