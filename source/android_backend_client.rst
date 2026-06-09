Android Backend Client
=======================

``BackendClient`` is the C++ class that runs on the Android tablet,
manages the TCP connection to the V3000, parses incoming status JSON,
and exposes all data to QML via Qt properties and signals.

BackendClient.h
----------------

.. code-block:: cpp

   // BackendClient.h
   #pragma once
   #include <QObject>
   #include <QTcpSocket>
   #include <QTimer>
   #include <QString>
   #include <QVariant>

   class BackendClient : public QObject
   {
       Q_OBJECT

       // ── Camera 1 properties ───────────────────────────────
       Q_PROPERTY(int    cam1Frames   READ cam1Frames   NOTIFY statusUpdated)
       Q_PROPERTY(double cam1Fps      READ cam1Fps      NOTIFY statusUpdated)
       Q_PROPERTY(double cam1Written  READ cam1Written  NOTIFY statusUpdated)
       Q_PROPERTY(double cam1Bw       READ cam1Bw       NOTIFY statusUpdated)
       Q_PROPERTY(int    cam1Dropped  READ cam1Dropped  NOTIFY statusUpdated)
       Q_PROPERTY(int    cam1BufFree  READ cam1BufFree  NOTIFY statusUpdated)
       Q_PROPERTY(bool   cam1GeoTag   READ cam1GeoTag   NOTIFY statusUpdated)

       // ── Camera 2 properties ───────────────────────────────
       Q_PROPERTY(int    cam2Frames   READ cam2Frames   NOTIFY statusUpdated)
       Q_PROPERTY(double cam2Fps      READ cam2Fps      NOTIFY statusUpdated)
       Q_PROPERTY(double cam2Written  READ cam2Written  NOTIFY statusUpdated)
       Q_PROPERTY(double cam2Bw       READ cam2Bw       NOTIFY statusUpdated)
       Q_PROPERTY(int    cam2Dropped  READ cam2Dropped  NOTIFY statusUpdated)
       Q_PROPERTY(int    cam2BufFree  READ cam2BufFree  NOTIFY statusUpdated)
       Q_PROPERTY(bool   cam2GeoTag   READ cam2GeoTag   NOTIFY statusUpdated)

       // ── GNSS properties ───────────────────────────────────
       Q_PROPERTY(bool   gnssValid    READ gnssValid    NOTIFY statusUpdated)
       Q_PROPERTY(QString gnssFix     READ gnssFix      NOTIFY statusUpdated)
       Q_PROPERTY(double gnssLat      READ gnssLat      NOTIFY statusUpdated)
       Q_PROPERTY(double gnssLon      READ gnssLon      NOTIFY statusUpdated)
       Q_PROPERTY(double gnssAlt      READ gnssAlt      NOTIFY statusUpdated)
       Q_PROPERTY(int    gnssSats     READ gnssSats     NOTIFY statusUpdated)
       Q_PROPERTY(double gnssHdop     READ gnssHdop     NOTIFY statusUpdated)
       Q_PROPERTY(QString gnssUtc     READ gnssUtc      NOTIFY statusUpdated)

       // ── Sync properties ───────────────────────────────────
       Q_PROPERTY(double syncDeltaMs  READ syncDeltaMs  NOTIFY statusUpdated)
       Q_PROPERTY(double syncTotalBw  READ syncTotalBw  NOTIFY statusUpdated)
       Q_PROPERTY(int    syncDropped  READ syncDropped  NOTIFY statusUpdated)

       // ── Connection state ──────────────────────────────────
       Q_PROPERTY(bool   connected    READ connected    NOTIFY connectionChanged)
       Q_PROPERTY(QString hostAddress READ hostAddress  NOTIFY connectionChanged)

   public:
       explicit BackendClient(QObject* parent = nullptr);

       // Property accessors
       int     cam1Frames()  const { return m_cam1.frames;    }
       double  cam1Fps()     const { return m_cam1.fps;       }
       double  cam1Written() const { return m_cam1.written_gb;}
       double  cam1Bw()      const { return m_cam1.bw_gbs;    }
       int     cam1Dropped() const { return m_cam1.dropped;   }
       int     cam1BufFree() const { return m_cam1.buf_free;  }
       bool    cam1GeoTag()  const { return m_cam1.geo_tagged;}

       int     cam2Frames()  const { return m_cam2.frames;    }
       double  cam2Fps()     const { return m_cam2.fps;       }
       double  cam2Written() const { return m_cam2.written_gb;}
       double  cam2Bw()      const { return m_cam2.bw_gbs;    }
       int     cam2Dropped() const { return m_cam2.dropped;   }
       int     cam2BufFree() const { return m_cam2.buf_free;  }
       bool    cam2GeoTag()  const { return m_cam2.geo_tagged;}

       bool    gnssValid()   const { return m_gnssValid;   }
       QString gnssFix()     const { return m_gnssFix;     }
       double  gnssLat()     const { return m_gnssLat;     }
       double  gnssLon()     const { return m_gnssLon;     }
       double  gnssAlt()     const { return m_gnssAlt;     }
       int     gnssSats()    const { return m_gnssSats;    }
       double  gnssHdop()    const { return m_gnssHdop;    }
       QString gnssUtc()     const { return m_gnssUtc;     }

       double  syncDeltaMs() const { return m_syncDeltaMs;  }
       double  syncTotalBw() const { return m_syncTotalBw;  }
       int     syncDropped() const { return m_syncDropped;  }

       bool    connected()   const { return m_connected;   }
       QString hostAddress() const { return m_host;        }

   public slots:
       // Called from QML
       Q_INVOKABLE void connectToV3000(const QString& host = "192.168.100.1",
                                       quint16 port = 9100);
       Q_INVOKABLE void disconnect();
       Q_INVOKABLE void triggerBoth();
       Q_INVOKABLE void triggerCam1();
       Q_INVOKABLE void triggerCam2();
       Q_INVOKABLE void startAll();
       Q_INVOKABLE void stopAll();
       Q_INVOKABLE void setExposure(int us);
       Q_INVOKABLE void setPixelFormat(const QString& fmt);
       Q_INVOKABLE void setOutputDir(const QString& dir);

   signals:
       void statusUpdated();
       void connectionChanged();
       void frameEvent(int cam, int index,
                       bool geoTagged, int processMs, QString ts);
       void errorOccurred(const QString& msg);

   private slots:
       void onConnected();
       void onDisconnected();
       void onReadyRead();
       void onSocketError(QAbstractSocket::SocketError err);
       void tryReconnect();

   private:
       void        sendCommand(const QJsonObject& cmd);
       void        parseStatus(const QJsonObject& obj);
       void        parseFrame(const QJsonObject& obj);

       struct CamStatus {
           int    frames = 0; double fps = 0; double written_gb = 0;
           double bw_gbs = 0; int dropped = 0; int buf_free = 0;
           bool   geo_tagged = false;
       };

       QTcpSocket* m_socket      = nullptr;
       QTimer*     m_reconnTimer = nullptr;
       QByteArray  m_readBuffer;
       QString     m_host;
       quint16     m_port        = 9100;
       bool        m_connected   = false;

       CamStatus   m_cam1, m_cam2;
       bool        m_gnssValid   = false;
       QString     m_gnssFix;
       double      m_gnssLat = 0, m_gnssLon = 0, m_gnssAlt = 0;
       int         m_gnssSats   = 0;
       double      m_gnssHdop   = 0;
       QString     m_gnssUtc;
       double      m_syncDeltaMs = 0, m_syncTotalBw = 0;
       int         m_syncDropped = 0;
   };

BackendClient.cpp (key methods)
---------------------------------

.. code-block:: cpp

   BackendClient::BackendClient(QObject* parent) : QObject(parent)
   {
       m_socket = new QTcpSocket(this);
       m_reconnTimer = new QTimer(this);
       m_reconnTimer->setInterval(3000);   // retry every 3s

       connect(m_socket, &QTcpSocket::connected,
               this,     &BackendClient::onConnected);
       connect(m_socket, &QTcpSocket::disconnected,
               this,     &BackendClient::onDisconnected);
       connect(m_socket, &QTcpSocket::readyRead,
               this,     &BackendClient::onReadyRead);
       connect(m_socket, &QAbstractSocket::errorOccurred,
               this,     &BackendClient::onSocketError);
       connect(m_reconnTimer, &QTimer::timeout,
               this,          &BackendClient::tryReconnect);
   }

   void BackendClient::connectToV3000(const QString& host, quint16 port)
   {
       m_host = host;
       m_port = port;
       m_socket->connectToHost(host, port);
       m_reconnTimer->start();
   }

   void BackendClient::onConnected()
   {
       m_connected = true;
       m_reconnTimer->stop();
       emit connectionChanged();
   }

   void BackendClient::onDisconnected()
   {
       m_connected = false;
       emit connectionChanged();
       m_reconnTimer->start();   // auto-reconnect
   }

   void BackendClient::tryReconnect()
   {
       if (!m_connected && !m_host.isEmpty())
           m_socket->connectToHost(m_host, m_port);
   }

   void BackendClient::onReadyRead()
   {
       m_readBuffer += m_socket->readAll();

       while (true)
       {
           int idx = m_readBuffer.indexOf('\n');
           if (idx < 0) break;

           QByteArray line = m_readBuffer.left(idx).trimmed();
           m_readBuffer.remove(0, idx + 1);
           if (line.isEmpty()) continue;

           QJsonDocument doc = QJsonDocument::fromJson(line);
           if (doc.isNull()) continue;

           QJsonObject obj = doc.object();
           QString type = obj["type"].toString();

           if      (type == "status") parseStatus(obj);
           else if (type == "frame")  parseFrame(obj);
       }
   }

   void BackendClient::parseStatus(const QJsonObject& obj)
   {
       auto parseCam = [](const QJsonObject& c, CamStatus& s) {
           s.frames     = c["frames"].toInt();
           s.fps        = c["fps"].toDouble();
           s.written_gb = c["written_gb"].toDouble();
           s.bw_gbs     = c["bw_gbs"].toDouble();
           s.dropped    = c["dropped"].toInt();
           s.buf_free   = c["buf_free"].toInt();
           s.geo_tagged = c["geo_tagged"].toBool();
       };

       parseCam(obj["cam1"].toObject(), m_cam1);
       parseCam(obj["cam2"].toObject(), m_cam2);

       QJsonObject g = obj["gnss"].toObject();
       m_gnssValid  = g["valid"].toBool();
       m_gnssFix    = g["fix"].toString();
       m_gnssLat    = g["lat"].toDouble();
       m_gnssLon    = g["lon"].toDouble();
       m_gnssAlt    = g["alt_m"].toDouble();
       m_gnssSats   = g["sats"].toInt();
       m_gnssHdop   = g["hdop"].toDouble();
       m_gnssUtc    = g["utc"].toString();

       QJsonObject s = obj["sync"].toObject();
       m_syncDeltaMs = s["delta_ms"].toDouble();
       m_syncTotalBw = s["total_bw_gbs"].toDouble();
       m_syncDropped = s["dropped_total"].toInt();

       emit statusUpdated();
   }

   void BackendClient::parseFrame(const QJsonObject& obj)
   {
       emit frameEvent(
           obj["cam"].toInt(),
           obj["index"].toInt(),
           obj["geo_tagged"].toBool(),
           obj["process_ms"].toInt(),
           obj["ts"].toString()
       );
   }

   void BackendClient::sendCommand(const QJsonObject& cmd)
   {
       if (!m_connected) return;
       m_socket->write(
           QJsonDocument(cmd).toJson(QJsonDocument::Compact) + '\n');
   }

   void BackendClient::triggerBoth()
   { sendCommand({{"type","trigger"},{"target","both"}}); }

   void BackendClient::triggerCam1()
   { sendCommand({{"type","trigger"},{"target","cam1"}}); }

   void BackendClient::triggerCam2()
   { sendCommand({{"type","trigger"},{"target","cam2"}}); }

   void BackendClient::startAll()
   { sendCommand({{"type","start"}}); }

   void BackendClient::stopAll()
   { sendCommand({{"type","stop"}}); }

   void BackendClient::setExposure(int us)
   { sendCommand({{"type","set"},{"param","exposure_us"},{"value",us}}); }

Registering BackendClient with QML
-------------------------------------

.. code-block:: cpp

   // android/main_android.cpp
   #include <QGuiApplication>
   #include <QQmlApplicationEngine>
   #include <QQmlContext>
   #include "BackendClient.h"

   int main(int argc, char* argv[])
   {
       QGuiApplication app(argc, argv);

       BackendClient backend;

       QQmlApplicationEngine engine;

       // Expose backend to all QML files as a global singleton
       engine.rootContext()->setContextProperty("backend", &backend);

       engine.load(QUrl("qrc:/SHRCamera/qml/MainView.qml"));

       // Auto-connect to V3000 on the USB NCM address
       backend.connectToV3000("192.168.100.1", 9100);

       return app.exec();
   }

In any QML file, ``backend`` is now accessible as a global object:

.. code-block:: qml

   // Any QML file — no import needed
   Text {
       text: backend.cam1Frames.toLocaleString()
   }
   Button {
       onClicked: backend.triggerBoth()
   }
   Text {
       text: backend.gnssValid
             ? backend.gnssLat.toFixed(6) + "°"
             : "No fix"
       color: backend.gnssValid ? "#5dddaa" : "#f5a623"
   }
