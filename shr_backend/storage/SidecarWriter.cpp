#include "SidecarWriter.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace SidecarWriter {

static QString fixDesc(int q)
{
    switch (q) {
        case 0: return "No fix";
        case 1: return "GPS fix";
        case 2: return "DGPS fix";
        case 4: return "RTK fixed";
        case 5: return "RTK float";
        default: return "Unknown";
    }
}

bool write(const QString& path, const FrameMetadata& meta)
{
    QJsonObject root;
    root["camera_index"]           = meta.camIndex;
    root["frame_index"]            = meta.frameIndex;
    root["frame_filename"]         = meta.imageFilename;
    root["capture_timestamp_utc"]  = meta.captureTime.toString(Qt::ISODateWithMs);
    root["camera_timestamp_ns"]    = static_cast<qint64>(meta.cameraTimestampNs);

    QJsonObject image;
    image["width"]            = static_cast<int>(meta.width);
    image["height"]           = static_cast<int>(meta.height);
    image["pixel_format_raw"] = meta.pixelFormatRaw;
    image["output_format"]    = "RGB8-PPM";
    root["image"] = image;

    QJsonObject gnss;
    if (meta.gnss && meta.gnss->valid) {
        const GnssRecord& g = *meta.gnss;
        qint64 latMs = g.host_timestamp.msecsTo(meta.captureTime);
        gnss["valid"]                       = true;
        gnss["fix_quality"]                 = g.fix_quality;
        gnss["fix_description"]             = fixDesc(g.fix_quality);
        gnss["latitude_deg"]                = g.latitude;
        gnss["longitude_deg"]               = g.longitude;
        gnss["altitude_m"]                  = g.altitude_m;
        gnss["speed_knots"]                 = g.speed_kn;
        gnss["track_true_deg"]              = g.track_deg;
        gnss["hdop"]                        = g.hdop;
        gnss["satellites_used"]             = g.satellites;
        gnss["nmea_utc_time"]               = g.utc_time;
        gnss["nmea_utc_date"]               = g.utc_date;
        gnss["gnss_fix_host_timestamp_utc"] = g.host_timestamp.toString(Qt::ISODateWithMs);
        gnss["latency_ms"]                  = static_cast<int>(latMs);
        gnss["raw_gga"]                     = g.raw_gga;
        gnss["raw_rmc"]                     = g.raw_rmc;
    } else {
        gnss["valid"]           = false;
        gnss["fix_quality"]     = 0;
        gnss["fix_description"] = "No fix";
    }
    root["gnss"] = gnss;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

} // namespace SidecarWriter
