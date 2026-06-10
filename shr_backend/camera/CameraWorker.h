#pragma once
#include <QObject>
#include <QDateTime>
#include <atomic>
#include <vector>
#include "VmbCPP/VmbCPP.h"

class CameraWorker;

class FrameObserver : public VmbCPP::IFrameObserver
{
public:
    FrameObserver(VmbCPP::CameraPtr camera, CameraWorker* worker)
        : IFrameObserver(camera), m_worker(worker) {}
    void FrameReceived(const VmbCPP::FramePtr pFrame) override;
private:
    CameraWorker* m_worker;
};

class CameraWorker : public QObject
{
    Q_OBJECT
public:
    explicit CameraWorker(const QString& cameraId,
                          int            camIndex,
                          const QString& outputDir,
                          QObject*       parent = nullptr);
    ~CameraWorker();

    void onFrameReceived(const VmbCPP::FramePtr& frame);

public slots:
    void start();
    void stop();
    void softwareTrigger();

signals:
    void frameReady(int frameIndex, bool geoTagged,
                    int processMs, const QString& ts);
    void metricsUpdated(double fps, qint64 bytesWritten,
                        int buffersFree, double bwGBs);
    void errorOccurred(const QString& msg);
    void statusChanged(const QString& msg);

private:
    void configureCamera();
    void setupBuffers();
    void teardown();
    void emitMetrics();

    QString                              m_cameraId;
    int                                  m_camIndex;
    QString                              m_outputDir;
    VmbCPP::CameraPtr                    m_camera;
    std::vector<VmbCPP::FramePtr>        m_frames;
    std::shared_ptr<FrameObserver>       m_observer;
    std::atomic<int>                     m_frameCount{0};
    std::atomic<qint64>                  m_bytesWritten{0};
    QDateTime                            m_startTime;

    static constexpr int BUFFER_COUNT = 5;
};
