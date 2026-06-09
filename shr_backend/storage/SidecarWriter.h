#pragma once
#include <QString>
#include <QDateTime>
#include <memory>
#include "../gnss/GnssRecord.h"

namespace SidecarWriter {

struct FrameMetadata {
    int                          camIndex;
    int                          frameIndex;
    QString                      imageFilename;
    QDateTime                    captureTime;
    uint64_t                     cameraTimestampNs;
    uint32_t                     width;
    uint32_t                     height;
    QString                      pixelFormatRaw;
    std::shared_ptr<GnssRecord>  gnss;
};

bool write(const QString& path, const FrameMetadata& meta);

} // namespace SidecarWriter
