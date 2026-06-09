#pragma once
#include <QString>
#include <QDateTime>
#include <memory>
#include <atomic>

struct GnssRecord
{
    double    latitude    = 0.0;
    double    longitude   = 0.0;
    double    altitude_m  = 0.0;
    double    speed_kn    = 0.0;
    double    track_deg   = 0.0;
    double    hdop        = 99.9;
    int       satellites  = 0;
    int       fix_quality = 0;
    QString   utc_time;
    QString   utc_date;
    QString   raw_gga;
    QString   raw_rmc;
    QDateTime host_timestamp;
    bool      valid       = false;
};

// Atomic shared pointer — written by NmeaReader,
// read from FrameObserver hot path (zero contention)
extern std::shared_ptr<GnssRecord> g_currentGnss;
