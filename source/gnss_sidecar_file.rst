JSON Sidecar File Writer
=========================

The worker thread processes each ``FramePayload``, transforms the image,
writes the image file, and then writes a companion JSON sidecar containing
all GNSS and frame metadata.

Sidecar File Format
--------------------

Each image ``frame_00042.ppm`` gets a companion ``frame_00042.json``:

.. code-block:: json

   {
     "frame_index": 42,
     "frame_filename": "frame_00042.ppm",
     "capture_timestamp_utc": "2026-06-07T14:23:11.847Z",
     "camera_timestamp_ns": 18374628374628,
     "image": {
       "width": 14192,
       "height": 10640,
       "pixel_format_raw": "BayerGR12",
       "output_format": "RGB8-PPM"
     },
     "gnss": {
       "valid": true,
       "fix_quality": 1,
       "fix_description": "GPS fix",
       "latitude_deg":  48.137154,
       "longitude_deg": 11.576124,
       "altitude_m":    524.3,
       "speed_knots":   0.0,
       "track_true_deg": 0.0,
       "hdop":          0.9,
       "satellites_used": 9,
       "nmea_utc_time": "142311.847",
       "nmea_utc_date": "070626",
       "gnss_fix_host_timestamp_utc": "2026-06-07T14:23:11.820Z",
       "latency_ms": 27,
       "raw_gga": "$GNGGA,142311.847,4808.2292,N,01134.5674,E,1,09,0.9,524.3,M,47.8,M,,*47",
       "raw_rmc": "$GNRMC,142311.847,A,4808.2292,N,01134.5674,E,0.00,0.00,070626,,,A*68"
     }
   }

SidecarWriter Implementation
------------------------------

.. code-block:: cpp

   // SidecarWriter.h
   #pragma once
   #include "NmeaReader.h"  // for GnssRecord
   #include <QString>
   #include <QDateTime>

   struct FrameMetadata
   {
       int                          frameIndex;
       QString                      imageFilename;
       QDateTime                    captureTime;
       uint64_t                     cameraTimestampNs;
       uint32_t                     width;
       uint32_t                     height;
       QString                      pixelFormatRaw;
       std::shared_ptr<GnssRecord>  gnss;
   };

   class SidecarWriter
   {
   public:
       static bool write(const QString& path,
                         const FrameMetadata& meta);

   private:
       static QString fixQualityDescription(int quality);
   };

.. code-block:: cpp

   // SidecarWriter.cpp
   #include "SidecarWriter.h"
   #include <QFile>
   #include <QJsonDocument>
   #include <QJsonObject>
   #include <QTextStream>

   bool SidecarWriter::write(const QString& path,
                              const FrameMetadata& meta)
   {
       QJsonObject root;

       root["frame_index"]            = meta.frameIndex;
       root["frame_filename"]         = meta.imageFilename;
       root["capture_timestamp_utc"]  =
           meta.captureTime.toString(Qt::ISODateWithMs);
       root["camera_timestamp_ns"]    =
           static_cast<qint64>(meta.cameraTimestampNs);

       // Image metadata
       QJsonObject image;
       image["width"]           = static_cast<int>(meta.width);
       image["height"]          = static_cast<int>(meta.height);
       image["pixel_format_raw"]= meta.pixelFormatRaw;
       image["output_format"]   = "RGB8-PPM";
       root["image"]            = image;

       // GNSS metadata
       QJsonObject gnss;
       if (meta.gnss && meta.gnss->valid)
       {
           const GnssRecord& g = *meta.gnss;

           // Latency: time between GNSS fix parse and frame capture
           qint64 latencyMs = g.host_timestamp.msecsTo(meta.captureTime);

           gnss["valid"]                      = true;
           gnss["fix_quality"]                = g.fix_quality;
           gnss["fix_description"]            =
               fixQualityDescription(g.fix_quality);
           gnss["latitude_deg"]               = g.latitude;
           gnss["longitude_deg"]              = g.longitude;
           gnss["altitude_m"]                 = g.altitude_m;
           gnss["speed_knots"]                = g.speed_kn;
           gnss["track_true_deg"]             = g.track_deg;
           gnss["hdop"]                       = g.hdop;
           gnss["satellites_used"]            = g.satellites;
           gnss["nmea_utc_time"]              = g.utc_time;
           gnss["nmea_utc_date"]              = g.utc_date;
           gnss["gnss_fix_host_timestamp_utc"]=
               g.host_timestamp.toString(Qt::ISODateWithMs);
           gnss["latency_ms"]                 =
               static_cast<int>(latencyMs);
           gnss["raw_gga"]                    = g.raw_gga;
           gnss["raw_rmc"]                    = g.raw_rmc;
       }
       else
       {
           gnss["valid"]         = false;
           gnss["fix_quality"]   = 0;
           gnss["fix_description"] = "No fix";
       }
       root["gnss"] = gnss;

       // Write to file
       QFile file(path);
       if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
           return false;

       QJsonDocument doc(root);
       file.write(doc.toJson(QJsonDocument::Indented));
       return true;
   }

   QString SidecarWriter::fixQualityDescription(int quality)
   {
       switch (quality)
       {
           case 0: return "No fix";
           case 1: return "GPS fix";
           case 2: return "DGPS fix";
           case 3: return "PPS fix";
           case 4: return "RTK fixed";
           case 5: return "RTK float";
           case 6: return "Dead reckoning";
           default: return "Unknown";
       }
   }

Worker Thread — Integrating the Sidecar
-----------------------------------------

The worker thread calls ``SidecarWriter::write()`` after saving the image:

.. code-block:: cpp

   void FrameWorker::processPayload(FramePayload payload)
   {
       // 1. Debayer
       std::vector<uint8_t> rgb(payload.width * payload.height * 3);
       transformFrame(payload.rawData.data(), payload.pixelFormat,
                      payload.width, payload.height, rgb);

       // 2. Build file paths
       QString baseName = QString("frame_%1")
           .arg(payload.frameIndex, 5, 10, QChar('0'));
       QString imagePath  = m_outputDir + "/" + baseName + ".ppm";
       QString sidecarPath= m_outputDir + "/" + baseName + ".json";

       // 3. Write image
       saveAsPPM(rgb, payload.width, payload.height, imagePath);

       // 4. Write JSON sidecar
       FrameMetadata meta;
       meta.frameIndex        = payload.frameIndex;
       meta.imageFilename     = baseName + ".ppm";
       meta.captureTime       = payload.captureTime;
       meta.cameraTimestampNs = payload.cameraTimestampNs;
       meta.width             = payload.width;
       meta.height            = payload.height;
       meta.pixelFormatRaw    = pixelFormatName(payload.pixelFormat);
       meta.gnss              = payload.gnss;

       SidecarWriter::write(sidecarPath, meta);

       // 5. Notify UI
       emit frameProcessed(payload.frameIndex,
                           payload.gnss ? payload.gnss->valid : false);
   }
