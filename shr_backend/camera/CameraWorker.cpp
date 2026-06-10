#include "CameraWorker.h"
#include "../gnss/GnssRecord.h"
#include "../storage/SidecarWriter.h"
#include "../transform/ImageTransform.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QElapsedTimer>

// ── FrameObserver ─────────────────────────────────────────────
void FrameObserver::FrameReceived(const VmbCPP::FramePtr pFrame)
{
    m_worker->onFrameReceived(pFrame);
}

// ── CameraWorker ──────────────────────────────────────────────
CameraWorker::CameraWorker(const QString& cameraId,
                           int            camIndex,
                           const QString& outputDir,
                           QObject*       parent)
    : QObject(parent)
    , m_cameraId(cameraId)
    , m_camIndex(camIndex)
    , m_outputDir(outputDir)
{
    QDir().mkpath(outputDir);
}

CameraWorker::~CameraWorker() { teardown(); }

void CameraWorker::start()
{
    emit statusChanged(QString("Camera %1: opening %2")
                       .arg(m_camIndex).arg(m_cameraId));

    VmbCPP::VmbSystem& sys = VmbCPP::VmbSystem::GetInstance();

    VmbError_t err = sys.OpenCameraByID(
        m_cameraId.toLocal8Bit().constData(),
        VmbAccessModeFull,
        m_camera);

    if (err != VmbErrorSuccess) {
        emit errorOccurred(QString("Camera %1: cannot open %2 (err=%3)")
                           .arg(m_camIndex).arg(m_cameraId).arg(err));
        return;
    }

    configureCamera();
    setupBuffers();

    VmbCPP::FeaturePtr feature;
    m_camera->GetFeatureByName("AcquisitionStart", feature);
    feature->RunCommand();

    m_startTime = QDateTime::currentDateTimeUtc();
    emit statusChanged(QString("Camera %1: acquiring").arg(m_camIndex));
}

void CameraWorker::stop()
{
    if (m_camera) {
        VmbCPP::FeaturePtr feature;
        m_camera->GetFeatureByName("AcquisitionStop", feature);
        if (feature) feature->RunCommand();
        m_camera->EndCapture();
        m_camera->FlushQueue();
        m_camera->RevokeAllFrames();
        m_camera->Close();
    }
    emit statusChanged(QString("Camera %1: stopped").arg(m_camIndex));
}

void CameraWorker::softwareTrigger()
{
    if (!m_camera) return;
    VmbCPP::FeaturePtr feature;
    if (VmbErrorSuccess == m_camera->GetFeatureByName("TriggerSoftware", feature))
        feature->RunCommand();
}

void CameraWorker::configureCamera()
{
    auto setEnum = [&](const char* name, const char* val) {
        VmbCPP::FeaturePtr f;
        if (VmbErrorSuccess == m_camera->GetFeatureByName(name, f))
            f->SetValue(val);
    };
    auto setInt = [&](const char* name, VmbInt64_t val) {
        VmbCPP::FeaturePtr f;
        if (VmbErrorSuccess == m_camera->GetFeatureByName(name, f))
            f->SetValue(val);
    };

    VmbInt64_t maxW = 0, maxH = 0;
    VmbCPP::FeaturePtr f;
    if (VmbErrorSuccess == m_camera->GetFeatureByName("WidthMax",  f)) f->GetValue(maxW);
    if (VmbErrorSuccess == m_camera->GetFeatureByName("HeightMax", f)) f->GetValue(maxH);
    if (maxW > 0) setInt("Width",  maxW);
    if (maxH > 0) setInt("Height", maxH);

    setEnum("PixelFormat",    "BayerGR12");
    setEnum("AcquisitionMode","Continuous");
    setEnum("TriggerSelector","FrameStart");
    setEnum("TriggerSource",  "Software");
    setEnum("TriggerMode",    "On");

    VmbInt64_t w = 0, h = 0;
    if (VmbErrorSuccess == m_camera->GetFeatureByName("Width",  f)) f->GetValue(w);
    if (VmbErrorSuccess == m_camera->GetFeatureByName("Height", f)) f->GetValue(h);
    emit statusChanged(QString("Camera %1: %2x%3 BayerGR12")
                       .arg(m_camIndex).arg(w).arg(h));
}

void CameraWorker::setupBuffers()
{
    VmbUint32_t payloadSize = 0;
    m_camera->GetPayloadSize(payloadSize);
    emit statusChanged(QString("Camera %1: payload %2 MB")
                       .arg(m_camIndex)
                       .arg(payloadSize / 1024 / 1024));

    m_observer = std::make_shared<FrameObserver>(m_camera, this);
    m_frames.resize(BUFFER_COUNT);

    for (auto& frame : m_frames) {
        frame.reset(new VmbCPP::Frame(payloadSize));
        frame->RegisterObserver(
            std::dynamic_pointer_cast<VmbCPP::IFrameObserver>(m_observer));
        m_camera->AnnounceFrame(frame);
    }

    m_camera->StartCapture();
    for (auto& frame : m_frames)
        m_camera->QueueFrame(frame);
}

void CameraWorker::teardown()
{
    stop();
    m_frames.clear();
    m_observer.reset();
}

void CameraWorker::onFrameReceived(const VmbCPP::FramePtr& pFrame)
{
    QElapsedTimer timer; timer.start();

    VmbFrameStatusType status;
    if (VmbErrorSuccess != pFrame->GetReceiveStatus(status) ||
        status != VmbFrameStatusComplete) {
        m_camera->QueueFrame(pFrame);
        return;
    }

    VmbUint32_t        width = 0, height = 0;
    VmbPixelFormatType fmt{};
    VmbUchar_t*        pData = nullptr;

    pFrame->GetWidth(width);
    pFrame->GetHeight(height);
    pFrame->GetPixelFormat(fmt);
    pFrame->GetImage(pData);

    int idx = ++m_frameCount;

    auto gnssSnapshot = std::atomic_load(&g_currentGnss);
    QDateTime captureTime = QDateTime::currentDateTimeUtc();

    // Transform Bayer → RGB8
    std::vector<uint8_t> rgb(width * height * 3);
    bool ok = ImageTransform::bayerToRGB8(
        pData,
        static_cast<VmbPixelFormat_t>(fmt),
        width, height, rgb);

    // Requeue immediately
    m_camera->QueueFrame(pFrame);

    if (!ok) return;

    QString baseName = QString("cam%1_frame_%2")
        .arg(m_camIndex)
        .arg(idx, 5, 10, QChar('0'));
    QString imagePath   = m_outputDir + "/" + baseName + ".ppm";
    QString sidecarPath = m_outputDir + "/" + baseName + ".json";

    // Write PPM
    QFile imgFile(imagePath);
    if (imgFile.open(QIODevice::WriteOnly)) {
        imgFile.write(
            QString("P6\n%1 %2\n255\n").arg(width).arg(height)
            .toLocal8Bit());
        imgFile.write(reinterpret_cast<const char*>(rgb.data()),
                      static_cast<qint64>(rgb.size()));
        imgFile.close();
        m_bytesWritten += static_cast<qint64>(rgb.size());
    }

    // Write JSON sidecar
    SidecarWriter::FrameMetadata meta;
    meta.camIndex         = m_camIndex;
    meta.frameIndex       = idx;
    meta.imageFilename    = baseName + ".ppm";
    meta.captureTime      = captureTime;
    meta.cameraTimestampNs= 0;
    meta.width            = width;
    meta.height           = height;
    meta.pixelFormatRaw   = "BayerGR12";
    meta.gnss             = gnssSnapshot;
    SidecarWriter::write(sidecarPath, meta);

    int elapsedMs = static_cast<int>(timer.elapsed());
    bool geoTagged = gnssSnapshot && gnssSnapshot->valid;

    qInfo() << QString("[Cam%1] #%2 %3x%4 %5 %6ms")
               .arg(m_camIndex).arg(idx)
               .arg(width).arg(height)
               .arg(geoTagged ? "geo" : "no-fix")
               .arg(elapsedMs);

    emitMetrics();
    emit frameReady(idx, geoTagged, elapsedMs,
                    captureTime.toString("HH:mm:ss.zzz"));
}

void CameraWorker::emitMetrics()
{
    double secs = m_startTime.secsTo(QDateTime::currentDateTimeUtc());
    double fps  = secs > 0 ? m_frameCount.load() / secs : 0.0;
    double bw   = secs > 0
                  ? (m_bytesWritten.load() / secs) / 1e9
                  : 0.0;
    emit metricsUpdated(fps, m_bytesWritten.load(), BUFFER_COUNT, bw);
}
