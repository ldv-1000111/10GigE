#include <QCoreApplication>
#include <QCommandLineParser>
#include <QThread>
#include <QTimer>
#include <csignal>

#include "../camera/CameraWorker.h"
#include "../gnss/NmeaReader.h"
#include "../trigger/TriggerServer.h"
#include "../server/BackendServer.h"

static QCoreApplication* g_app = nullptr;
void sigHandler(int) { if (g_app) g_app->quit(); }

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    g_app = &app;
    app.setApplicationName("SHR-Backend");
    app.setApplicationVersion("1.0");

    std::signal(SIGINT,  sigHandler);
    std::signal(SIGTERM, sigHandler);

    // ── CLI arguments ────────────────────────────────────────
    QCommandLineParser parser;
    parser.setApplicationDescription("SHR Camera Acquisition Backend");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption camSimOpt(
        {"s", "simulator"},
        "Use Vimba X Camera Simulator (no real cameras needed)");
    QCommandLineOption cam1Opt(
        "cam1", "Camera 1 ID or IP (default: first found)", "cam1", "");
    QCommandLineOption cam2Opt(
        "cam2", "Camera 2 ID or IP (default: second found)", "cam2", "");
    QCommandLineOption gnssOpt(
        "gnss-port", "GNSS serial port (default: /dev/ttyUSB0)", "port", "/dev/ttyUSB0");
    QCommandLineOption gnssBaudOpt(
        "gnss-baud", "GNSS baud rate (default: 9600)", "baud", "9600");
    QCommandLineOption outDirOpt(
        "output", "Frame output directory (default: ~/frames)", "dir",
        QDir::homePath() + "/frames");
    QCommandLineOption tcpPortOpt(
        "port", "Backend TCP port (default: 9100)", "port", "9100");

    parser.addOption(camSimOpt);
    parser.addOption(cam1Opt);
    parser.addOption(cam2Opt);
    parser.addOption(gnssOpt);
    parser.addOption(gnssBaudOpt);
    parser.addOption(outDirOpt);
    parser.addOption(tcpPortOpt);
    parser.process(app);

    bool    useSimulator = parser.isSet(camSimOpt);
    QString cam1Id       = parser.value(cam1Opt);
    QString cam2Id       = parser.value(cam2Opt);
    QString gnssPort     = parser.value(gnssOpt);
    int     gnssBaud     = parser.value(gnssBaudOpt).toInt();
    QString outputDir    = parser.value(outDirOpt);
    quint16 tcpPort      = static_cast<quint16>(parser.value(tcpPortOpt).toUInt());

    qInfo() << "===========================================";
    qInfo() << " SHR Backend 1.0";
    qInfo() << " Simulator:  " << (useSimulator ? "yes" : "no");
    qInfo() << " GNSS port:  " << gnssPort << "@ " << gnssBaud;
    qInfo() << " Output dir: " << outputDir;
    qInfo() << " TCP port:   " << tcpPort;
    qInfo() << "===========================================";

    // ── Vimba X startup ──────────────────────────────────────
    VmbCPP::VmbSystem& vmb = VmbCPP::VmbSystem::GetInstance();
    VmbErrorT err = vmb.Startup();
    if (err != VmbErrorSuccess) {
        qCritical() << "Vimba X startup failed:" << err;
        return 1;
    }
    qInfo() << "Vimba X started";

    // If using simulator, enumerate after small delay
    if (useSimulator) {
        qInfo() << "Camera Simulator mode — enumerating...";
        QThread::msleep(500);
    }

    // Auto-discover cameras if no IDs given
    if (cam1Id.isEmpty() || cam2Id.isEmpty()) {
        VmbCPP::CameraPtrVector cameras;
        vmb.GetCameras(cameras);
        qInfo() << "Cameras found:" << cameras.size();
        for (size_t i = 0; i < cameras.size(); ++i) {
            std::string id, name;
            cameras[i]->GetID(id);
            cameras[i]->GetName(name);
            qInfo() << "  [" << i << "]" << QString::fromStdString(id)
                    << QString::fromStdString(name);
        }
        if (cameras.size() >= 1 && cam1Id.isEmpty()) {
            std::string id; cameras[0]->GetID(id);
            cam1Id = QString::fromStdString(id);
        }
        if (cameras.size() >= 2 && cam2Id.isEmpty()) {
            std::string id; cameras[1]->GetID(id);
            cam2Id = QString::fromStdString(id);
        }
    }

    if (cam1Id.isEmpty()) {
        qCritical() << "No cameras found. Use --simulator or connect a camera.";
        vmb.Shutdown();
        return 1;
    }

    // ── GNSS reader ──────────────────────────────────────────
    auto* gnssThread = new QThread(&app);
    auto* nmea       = new NmeaReader();
    nmea->moveToThread(gnssThread);
    QObject::connect(gnssThread, &QThread::started, nmea, [=]{
        nmea->open(gnssPort, gnssBaud);
    });
    QObject::connect(nmea, &NmeaReader::statusChanged,
                     &app, [](const QString& s){ qInfo() << "[GNSS]" << s; });
    QObject::connect(nmea, &NmeaReader::portError,
                     &app, [](const QString& e){ qWarning() << "[GNSS] ERROR:" << e; });
    gnssThread->start();

    // ── Camera workers ───────────────────────────────────────
    auto* t1 = new QThread(&app);
    auto* t2 = cam2Id.isEmpty() ? nullptr : new QThread(&app);

    auto* w1 = new CameraWorker(cam1Id, 1, outputDir);
    auto* w2 = cam2Id.isEmpty() ? nullptr : new CameraWorker(cam2Id, 2, outputDir);

    w1->moveToThread(t1);
    if (w2) w2->moveToThread(t2);

    // ── Trigger server ───────────────────────────────────────
    auto* trigServer = new TriggerServer(9001, &app);
    trigServer->start();

    // ── Backend server ───────────────────────────────────────
    auto* backend = new BackendServer(tcpPort, &app);
    backend->start();

    // ── Wire camera metrics → backend ────────────────────────
    auto wireWorker = [&](CameraWorker* w, int idx) {
        if (!w) return;
        QObject::connect(w, &CameraWorker::metricsUpdated,
            &app, [=](double fps, qint64 written, int bufFree, double bw) {
                BackendServer::CameraStatus s;
                s.camIndex = idx; s.fps = fps;
                s.written_gb = written / 1e9;
                s.bw_gbs = bw; s.buf_free = bufFree;
                backend->updateCameraStatus(idx, s);
            });
        QObject::connect(w, &CameraWorker::frameReady,
            backend, [=](int fi, bool geo, int ms, QString ts) {
                backend->onFrameProcessed(idx, geo, ms, ts);
            });
        QObject::connect(w, &CameraWorker::errorOccurred,
            &app, [=](const QString& e) {
                qCritical() << QString("[Cam%1] ERROR: %2").arg(idx).arg(e);
            });
        QObject::connect(w, &CameraWorker::statusChanged,
            &app, [=](const QString& s) {
                qInfo() << s;
            });
    };
    wireWorker(w1, 1);
    wireWorker(w2, 2);

    // ── Wire backend commands → cameras ──────────────────────
    QObject::connect(backend, &BackendServer::triggerRequested,
        &app, [=](int cam) {
            if ((cam == 0 || cam == 1) && w1) w1->softwareTrigger();
            if ((cam == 0 || cam == 2) && w2) w2->softwareTrigger();
        });
    QObject::connect(backend, &BackendServer::startRequested,
        &app, [=]() {
            if (w1) w1->start();
            if (w2) w2->start();
        });
    QObject::connect(backend, &BackendServer::stopRequested,
        &app, [=]() {
            if (w1) w1->stop();
            if (w2) w2->stop();
        });

    // ── Wire UDP trigger ─────────────────────────────────────
    QObject::connect(trigServer, &TriggerServer::triggerReceived,
        &app, [=]() {
            if (w1) w1->softwareTrigger();
            if (w2) w2->softwareTrigger();
        });

    // ── Start threads ─────────────────────────────────────────
    QObject::connect(t1, &QThread::started, w1, &CameraWorker::start);
    if (t2 && w2)
        QObject::connect(t2, &QThread::started, w2, &CameraWorker::start);

    t1->start();
    if (t2) t2->start();

    qInfo() << "SHR Backend running.";
    qInfo() << "  TCP status stream: :" << tcpPort;
    qInfo() << "  UDP trigger:       :9001";
    qInfo() << "  Press Ctrl+C to stop.";

    int ret = app.exec();

    // ── Clean shutdown ────────────────────────────────────────
    qInfo() << "Shutting down...";
    if (w1) { w1->stop(); t1->quit(); t1->wait(3000); }
    if (w2) { w2->stop(); t2->quit(); t2->wait(3000); }
    gnssThread->quit(); gnssThread->wait(2000);
    vmb.Shutdown();
    qInfo() << "Done.";
    return ret;
}
