Complete Application — Source Listings
========================================

This chapter provides the complete annotated source listings for both
processes of the application. Each file corresponds to a chapter in
this tutorial.

The application comprises two independent binaries:

.. list-table::
   :header-rows: 1
   :widths: 30 30 40

   * - Binary
     - Runs on
     - Entry point
   * - ``SHR_Backend``
     - Bedrock V3000
     - ``main_v3000.cpp`` (``QCoreApplication``)
   * - ``SHR_Camera_Android``
     - Android tablet
     - ``main_android.cpp`` (``QGuiApplication``)

----

Part A — V3000 Backend
------------------------

File structure
^^^^^^^^^^^^^^

.. code-block:: text

   shr_backend/
   ├── main_v3000.cpp              ← QCoreApplication entry point
   │
   ├── camera/
   │   ├── CameraWorker.h/.cpp     ← Vimba X per-camera acquisition thread
   │   └── FramePayload.h          ← frame + GNSS bundle passed to worker
   │
   ├── gnss/
   │   ├── NmeaReader.h/.cpp       ← QSerialPort + NMEA parser
   │   └── GnssRecord.h            ← atomic shared GNSS data structure
   │
   ├── transform/
   │   └── ImageTransform.h/.cpp   ← VmbImageTransform wrapper
   │
   ├── storage/
   │   └── SidecarWriter.h/.cpp    ← JSON sidecar file writer
   │
   ├── trigger/
   │   └── TriggerServer.h/.cpp    ← UDP :9001 Action Command listener
   │
   ├── server/
   │   └── BackendServer.h/.cpp    ← TCP :9100 Android client server
   │
   └── systemd/
       └── shr-backend.service     ← systemd unit for autostart at boot

main_v3000.cpp
^^^^^^^^^^^^^^

.. code-block:: cpp

   // main_v3000.cpp — V3000 headless acquisition daemon
   // QCoreApplication: no display, no UI dependency
   #include <QCoreApplication>
   #include <QThread>
   #include <QTimer>
   #include <csignal>

   #include "camera/CameraWorker.h"
   #include "gnss/NmeaReader.h"
   #include "trigger/TriggerServer.h"
   #include "server/BackendServer.h"

   static QCoreApplication* g_app = nullptr;

   void signalHandler(int)
   {
       if (g_app) g_app->quit();
   }

   int main(int argc, char* argv[])
   {
       QCoreApplication app(argc, argv);
       g_app = &app;

       app.setApplicationName("SHR-Backend");
       app.setApplicationVersion("1.0");

       std::signal(SIGINT,  signalHandler);
       std::signal(SIGTERM, signalHandler);

       qInfo() << "SHR Backend starting...";

       // ── GNSS reader ────────────────────────────────────────────
       auto* gnssThread = new QThread(&app);
       auto* nmea       = new NmeaReader();
       nmea->moveToThread(gnssThread);

       QObject::connect(gnssThread, &QThread::started, nmea,
           [nmea]{ nmea->open("/dev/ttyUSB0", 9600); });
       gnssThread->start();

       // ── Camera workers ─────────────────────────────────────────
       auto* thread1 = new QThread(&app);
       auto* thread2 = new QThread(&app);
       auto* worker1 = new CameraWorker("192.168.10.41");
       auto* worker2 = new CameraWorker("192.168.10.42");
       worker1->moveToThread(thread1);
       worker2->moveToThread(thread2);

       // ── Trigger server (UDP :9001) ─────────────────────────────
       auto* trigServer = new TriggerServer(9001, &app);
       trigServer->start();

       // ── Backend server (TCP :9100) ─────────────────────────────
       auto* backend = new BackendServer(9100, &app);
       backend->start();

       // ── Wire camera metrics → backend status ───────────────────
       auto wireMetrics = [backend](CameraWorker* w, int idx) {
           QObject::connect(w, &CameraWorker::metricsUpdated,
               backend, [backend, idx](double fps, qint64 written,
                                       int bufFree, double bw) {
                   BackendServer::CameraStatus s;
                   s.fps = fps; s.written_gb = written / 1e9;
                   s.bw_gbs = bw; s.buf_free = bufFree;
                   backend->updateCameraStatus(idx, s);
               });
       };
       wireMetrics(worker1, 1);
       wireMetrics(worker2, 2);

       // ── Wire frame events → backend log ────────────────────────
       auto wireFrames = [backend](CameraWorker* w, int idx) {
           QObject::connect(w, &CameraWorker::frameReady,
               backend, [backend, idx](int fi, QImage, bool geo) {
                   backend->onFrameProcessed(idx, fi, geo, 0,
                       QDateTime::currentDateTimeUtc()
                           .toString("HH:mm:ss.zzz"));
               });
       };
       wireFrames(worker1, 1);
       wireFrames(worker2, 2);

       // ── Wire backend commands → camera workers ─────────────────
       QObject::connect(backend, &BackendServer::triggerRequested,
           &app, [worker1, worker2](int cam) {
               if (cam == 0 || cam == 1) worker1->softwareTrigger();
               if (cam == 0 || cam == 2) worker2->softwareTrigger();
           });

       QObject::connect(backend, &BackendServer::startRequested,
           &app, [worker1, worker2]() {
               worker1->start(); worker2->start();
           });

       QObject::connect(backend, &BackendServer::stopRequested,
           &app, [worker1, worker2]() {
               worker1->stop(); worker2->stop();
           });

       // ── Wire external UDP trigger ──────────────────────────────
       QObject::connect(trigServer, &TriggerServer::triggerReceived,
           &app, [worker1, worker2]() {
               worker1->softwareTrigger();
               worker2->softwareTrigger();
           });

       // ── Start camera threads ───────────────────────────────────
       QObject::connect(thread1, &QThread::started,
                        worker1, &CameraWorker::start);
       QObject::connect(thread2, &QThread::started,
                        worker2, &CameraWorker::start);
       thread1->start();
       thread2->start();

       qInfo() << "SHR Backend running — waiting for Android client on TCP :9100";

       int ret = app.exec();

       // ── Clean shutdown ─────────────────────────────────────────
       worker1->stop(); worker2->stop();
       thread1->quit(); thread1->wait();
       thread2->quit(); thread2->wait();
       gnssThread->quit(); gnssThread->wait();

       qInfo() << "SHR Backend stopped.";
       return ret;
   }

shr-backend.service — systemd unit
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The V3000 backend runs as a systemd service, starting automatically at
boot before any user session:

.. code-block:: ini

   # /etc/systemd/system/shr-backend.service
   [Unit]
   Description=SHR 10GigE Acquisition Backend
   After=network.target usb-gadget.service
   Requires=usb-gadget.service

   [Service]
   Type=simple
   User=luis
   WorkingDirectory=/home/luis
   ExecStart=/usr/local/bin/SHR_Backend
   Restart=on-failure
   RestartSec=3s
   Environment="LD_LIBRARY_PATH=/home/lvs/VimbaX_2026-1/api/lib/x86_64"
   Environment="GENICAM_GENTL64_PATH=/home/lvs/VimbaX_2026-1/cti"

   # Real-time scheduling permission
   LimitRTPRIO=99
   LimitMEMLOCK=infinity

   # Logging
   StandardOutput=journal
   StandardError=journal
   SyslogIdentifier=shr-backend

   [Install]
   WantedBy=multi-user.target

Install and enable:

.. code-block:: bash

   # Copy binary to system path
   sudo cp build/SHR_Backend /usr/local/bin/
   sudo chmod +x /usr/local/bin/SHR_Backend

   # Install and enable the service
   sudo cp systemd/shr-backend.service /etc/systemd/system/
   sudo systemctl daemon-reload
   sudo systemctl enable shr-backend
   sudo systemctl start shr-backend

   # Check status
   sudo systemctl status shr-backend
   journalctl -u shr-backend -f

----

Part B — Android Frontend
--------------------------

File structure
^^^^^^^^^^^^^^

.. code-block:: text

   shr_android/
   ├── main_android.cpp            ← QGuiApplication entry point
   ├── BackendClient.h/.cpp        ← TCP client, Q_PROPERTY → QML
   │
   └── qml/
       ├── MainView.qml            ← root ApplicationWindow
       ├── CameraPanel.qml         ← per-camera preview + metrics
       ├── TriggerBar.qml          ← trigger + start/stop buttons
       ├── RightPanel.qml          ← GNSS + sync + log container
       ├── GnssPanel.qml           ← coordinate display
       ├── SyncPanel.qml           ← sync metrics 2×2 grid
       ├── SectionWidget.qml       ← collapsible section
       ├── FrameLog.qml            ← scrollable frame log
       └── styles/
           └── colors.js           ← shared colour palette

main_android.cpp
^^^^^^^^^^^^^^^^

.. code-block:: cpp

   // main_android.cpp — Android Qt entry point
   #include <QGuiApplication>
   #include <QQmlApplicationEngine>
   #include <QQmlContext>
   #include "BackendClient.h"

   int main(int argc, char* argv[])
   {
       QGuiApplication app(argc, argv);
       app.setApplicationName("SHR Camera Control");
       app.setApplicationVersion("1.0");

       BackendClient backend;

       QQmlApplicationEngine engine;

       // Expose backend singleton to all QML files
       engine.rootContext()->setContextProperty("backend", &backend);

       const QUrl url("qrc:/SHRCamera/qml/MainView.qml");
       QObject::connect(&engine,
           &QQmlApplicationEngine::objectCreated,
           &app, [url](QObject* obj, const QUrl& objUrl) {
               if (!obj && url == objUrl)
                   QCoreApplication::exit(-1);
           }, Qt::QueuedConnection);

       engine.load(url);

       // Auto-connect to V3000 on USB NCM fixed address
       backend.connectToV3000("192.168.100.1", 9100);

       return app.exec();
   }

RightPanel.qml
^^^^^^^^^^^^^^

.. code-block:: qml

   // qml/RightPanel.qml
   import QtQuick 2.15
   import QtQuick.Layouts 1.15
   import "styles/colors.js" as C

   Rectangle {
       color: C.bg1
       border.color: "#2a2d31"; border.width: 0

       // Left border
       Rectangle {
           anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
           width: 1; color: "#2a2d31"
       }

       ColumnLayout {
           anchors.fill: parent
           spacing: 0

           // GNSS — always visible, no collapse
           GnssPanel {
               Layout.fillWidth: true
           }

           // Sync — always visible
           SyncPanel {
               Layout.fillWidth: true
           }

           // Camera config — collapsed by default
           SectionWidget {
               Layout.fillWidth: true
               title: "Camera config"
               expanded: false
               content: CameraConfigBody {}
           }

           // Trigger config — collapsed by default
           SectionWidget {
               Layout.fillWidth: true
               title: "Trigger config"
               expanded: false
               content: TriggerConfigBody {}
           }

           // Acquisition — collapsed by default
           SectionWidget {
               Layout.fillWidth: true
               title: "Acquisition"
               expanded: false
               content: AcquisitionBody {}
           }

           // Frame log — expanded, fills remaining space
           SectionWidget {
               Layout.fillWidth: true
               Layout.fillHeight: true
               title: "Frame log"
               expanded: true
               content: FrameLog {}
           }
       }
   }
