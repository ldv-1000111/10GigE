V3000 Backend Server
=====================

The ``BackendServer`` class is the bridge between the C++ acquisition
pipeline and the Android app. It runs a ``QTcpServer`` on port 9100,
accepts one connection from the Android tablet, pushes status JSON at
10 Hz, and dispatches incoming command JSON to the acquisition pipeline.

BackendServer.h
----------------

.. code-block:: cpp

   // BackendServer.h
   #pragma once
   #include <QObject>
   #include <QTcpServer>
   #include <QTcpSocket>
   #include <QTimer>
   #include <QJsonObject>
   #include <memory>
   #include "GnssRecord.h"

   struct CameraStatus
   {
       int     frames     = 0;
       double  fps        = 0.0;
       double  written_gb = 0.0;
       double  bw_gbs     = 0.0;
       int     dropped    = 0;
       int     buf_free   = 0;
       bool    geo_tagged = false;
   };

   class BackendServer : public QObject
   {
       Q_OBJECT
   public:
       explicit BackendServer(quint16 port = 9100,
                              QObject* parent = nullptr);

       bool start();
       void stop();

       // Called by CameraWorker threads to update status
       void updateCameraStatus(int camIndex,
                               const CameraStatus& status);

   signals:
       // Emitted when Android sends a command
       void triggerRequested(int camIndex);   // 0 = both, 1 = cam1, 2 = cam2
       void startRequested();
       void stopRequested();
       void paramChanged(const QString& param, const QVariant& value);

       // Emitted for UI feedback
       void clientConnected();
       void clientDisconnected();

   public slots:
       void onFrameProcessed(int camIndex, int frameIndex,
                             bool geoTagged, int processMs,
                             const QString& ts);

   private slots:
       void onNewConnection();
       void onClientDisconnected();
       void onReadyRead();
       void pushStatus();

   private:
       void        handleCommand(const QJsonObject& cmd);
       void        send(const QJsonObject& obj);
       QJsonObject buildStatusPayload() const;

       QTcpServer*   m_server   = nullptr;
       QTcpSocket*   m_client   = nullptr;   // one client at a time
       QTimer*       m_pushTimer= nullptr;
       quint16       m_port;

       CameraStatus  m_cam1;
       CameraStatus  m_cam2;
       QByteArray    m_readBuffer;
   };

BackendServer.cpp
------------------

.. code-block:: cpp

   // BackendServer.cpp
   #include "BackendServer.h"
   #include <QTcpSocket>
   #include <QJsonDocument>
   #include <QJsonArray>
   #include <QDateTime>

   BackendServer::BackendServer(quint16 port, QObject* parent)
       : QObject(parent), m_port(port)
   {
       m_server    = new QTcpServer(this);
       m_pushTimer = new QTimer(this);
       m_pushTimer->setInterval(100);   // 10 Hz

       connect(m_server,    &QTcpServer::newConnection,
               this,        &BackendServer::onNewConnection);
       connect(m_pushTimer, &QTimer::timeout,
               this,        &BackendServer::pushStatus);
   }

   bool BackendServer::start()
   {
       if (!m_server->listen(QHostAddress::Any, m_port))
       {
           qWarning() << "BackendServer: cannot listen on port" << m_port;
           return false;
       }
       qInfo() << "BackendServer: listening on port" << m_port;
       return true;
   }

   void BackendServer::stop()
   {
       m_pushTimer->stop();
       if (m_client) m_client->disconnectFromHost();
       m_server->close();
   }

   void BackendServer::onNewConnection()
   {
       // Accept only one client — disconnect any existing one
       if (m_client)
       {
           m_client->disconnectFromHost();
           m_client->deleteLater();
       }

       m_client = m_server->nextPendingConnection();

       connect(m_client, &QTcpSocket::disconnected,
               this,     &BackendServer::onClientDisconnected);
       connect(m_client, &QTcpSocket::readyRead,
               this,     &BackendServer::onReadyRead);

       m_pushTimer->start();
       emit clientConnected();
       qInfo() << "BackendServer: Android client connected from"
               << m_client->peerAddress().toString();
   }

   void BackendServer::onClientDisconnected()
   {
       m_pushTimer->stop();
       m_client->deleteLater();
       m_client = nullptr;
       emit clientDisconnected();
       qInfo() << "BackendServer: Android client disconnected";
   }

   void BackendServer::onReadyRead()
   {
       m_readBuffer += m_client->readAll();

       // Messages are newline-delimited JSON
       while (true)
       {
           int idx = m_readBuffer.indexOf('\n');
           if (idx < 0) break;

           QByteArray line = m_readBuffer.left(idx).trimmed();
           m_readBuffer.remove(0, idx + 1);

           if (line.isEmpty()) continue;

           QJsonParseError err;
           QJsonDocument doc = QJsonDocument::fromJson(line, &err);
           if (err.error != QJsonParseError::NoError)
           {
               qWarning() << "BackendServer: JSON parse error:" << err.errorString();
               continue;
           }
           handleCommand(doc.object());
       }
   }

   void BackendServer::handleCommand(const QJsonObject& cmd)
   {
       const QString type = cmd["type"].toString();

       if (type == "trigger")
       {
           const QString target = cmd["target"].toString("both");
           if      (target == "both") emit triggerRequested(0);
           else if (target == "cam1") emit triggerRequested(1);
           else if (target == "cam2") emit triggerRequested(2);
       }
       else if (type == "start")
       {
           emit startRequested();
       }
       else if (type == "stop")
       {
           emit stopRequested();
       }
       else if (type == "set")
       {
           emit paramChanged(cmd["param"].toString(),
                             cmd["value"].toVariant());
       }
   }

   void BackendServer::pushStatus()
   {
       if (!m_client || m_client->state() != QAbstractSocket::ConnectedState)
           return;

       send(buildStatusPayload());
   }

   QJsonObject BackendServer::buildStatusPayload() const
   {
       auto camToJson = [](const CameraStatus& c) -> QJsonObject {
           return {
               { "frames",     c.frames     },
               { "fps",        c.fps        },
               { "written_gb", c.written_gb },
               { "bw_gbs",     c.bw_gbs     },
               { "dropped",    c.dropped    },
               { "buf_free",   c.buf_free   },
               { "geo_tagged", c.geo_tagged }
           };
       };

       auto gnss = std::atomic_load(&g_currentGnss);
       QJsonObject gnssObj;
       if (gnss && gnss->valid)
       {
           gnssObj = {
               { "valid",  true                 },
               { "fix",    gnss->fix_quality > 0
                           ? "GPS 3D" : "No fix" },
               { "lat",    gnss->latitude       },
               { "lon",    gnss->longitude      },
               { "alt_m",  gnss->altitude_m     },
               { "sats",   gnss->satellites     },
               { "hdop",   gnss->hdop           },
               { "utc",    gnss->utc_time       }
           };
       }
       else
       {
           gnssObj = { { "valid", false } };
       }

       return {
           { "type",  "status"              },
           { "cam1",  camToJson(m_cam1)     },
           { "cam2",  camToJson(m_cam2)     },
           { "gnss",  gnssObj               },
           { "sync",  QJsonObject{
               { "delta_ms",      0.8  },
               { "total_bw_gbs",  m_cam1.bw_gbs + m_cam2.bw_gbs },
               { "dropped_total", m_cam1.dropped + m_cam2.dropped }
           }}
       };
   }

   void BackendServer::updateCameraStatus(int camIndex,
                                          const CameraStatus& status)
   {
       if      (camIndex == 1) m_cam1 = status;
       else if (camIndex == 2) m_cam2 = status;
   }

   void BackendServer::onFrameProcessed(int camIndex, int frameIndex,
                                        bool geoTagged, int processMs,
                                        const QString& ts)
   {
       if (!m_client) return;

       send({
           { "type",       "frame"     },
           { "cam",        camIndex    },
           { "index",      frameIndex  },
           { "geo_tagged", geoTagged   },
           { "process_ms", processMs   },
           { "ts",         ts          }
       });
   }

   void BackendServer::send(const QJsonObject& obj)
   {
       if (!m_client) return;
       QByteArray data = QJsonDocument(obj).toJson(
           QJsonDocument::Compact) + '\n';
       m_client->write(data);
   }

Wiring BackendServer into main()
----------------------------------

.. code-block:: cpp

   // main.cpp — V3000 headless backend
   #include <QCoreApplication>
   #include "BackendServer.h"
   #include "camera/CameraWorker.h"
   #include "gnss/NmeaReader.h"

   int main(int argc, char* argv[])
   {
       QCoreApplication app(argc, argv);

       // Start GNSS reader
       auto* gnssThread = new QThread(&app);
       auto* nmea       = new NmeaReader();
       nmea->moveToThread(gnssThread);
       QObject::connect(gnssThread, &QThread::started,
                        nmea, [nmea]{ nmea->open("/dev/ttyUSB0", 9600); });
       gnssThread->start();

       // Start camera workers
       auto* worker1 = new CameraWorker("192.168.10.41", &app);
       auto* worker2 = new CameraWorker("192.168.10.42", &app);

       // Start backend server
       auto* server = new BackendServer(9100, &app);
       server->start();

       // Wire camera metrics → server status updates
       QObject::connect(worker1, &CameraWorker::metricsUpdated,
           &app, [server](double fps, qint64 written,
                          int bufFree, double bw) {
               BackendServer::CameraStatus s;
               s.fps = fps; s.written_gb = written / 1e9;
               s.bw_gbs = bw; s.buf_free = bufFree;
               server->updateCameraStatus(1, s);
       });

       QObject::connect(worker2, &CameraWorker::metricsUpdated,
           &app, [server](double fps, qint64 written,
                          int bufFree, double bw) {
               BackendServer::CameraStatus s;
               s.fps = fps; s.written_gb = written / 1e9;
               s.bw_gbs = bw; s.buf_free = bufFree;
               server->updateCameraStatus(2, s);
       });

       // Wire frame events → server frame log
       QObject::connect(worker1, &CameraWorker::frameReady,
           server, [server](int idx, QImage, bool geo) {
               server->onFrameProcessed(1, idx, geo, 0,
                   QDateTime::currentDateTimeUtc()
                       .toString("HH:mm:ss.zzz"));
       });

       // Wire server commands → camera workers
       QObject::connect(server, &BackendServer::triggerRequested,
           &app, [worker1, worker2](int cam) {
               if (cam == 0 || cam == 1) worker1->softwareTrigger();
               if (cam == 0 || cam == 2) worker2->softwareTrigger();
       });

       QObject::connect(server, &BackendServer::startRequested,
           &app, [worker1, worker2]() {
               worker1->start(); worker2->start();
       });

       QObject::connect(server, &BackendServer::stopRequested,
           &app, [worker1, worker2]() {
               worker1->stop(); worker2->stop();
       });

       return app.exec();
   }

.. note::
   The V3000 backend uses ``QCoreApplication`` — not ``QApplication``.
   There is no UI on the V3000. This means the V3000 binary has no
   display dependency and runs headlessly at boot as a systemd service.
