#include "BackendServer.h"
#include <QJsonDocument>
#include <QDebug>

BackendServer::BackendServer(quint16 port, QObject* parent)
    : QObject(parent), m_port(port)
{
    m_cam1.camIndex = 1;
    m_cam2.camIndex = 2;

    m_server = new QTcpServer(this);
    m_timer  = new QTimer(this);
    m_timer->setInterval(100);  // 10 Hz

    connect(m_server, &QTcpServer::newConnection,
            this, &BackendServer::onNewConnection);
    connect(m_timer,  &QTimer::timeout,
            this, &BackendServer::pushStatus);
}

bool BackendServer::start()
{
    if (!m_server->listen(QHostAddress::Any, m_port)) {
        qWarning() << "[Server] Cannot listen on port" << m_port;
        return false;
    }
    qInfo() << "[Server] Listening on TCP :" << m_port;
    return true;
}

void BackendServer::stop()
{
    m_timer->stop();
    if (m_client) m_client->disconnectFromHost();
    m_server->close();
}

void BackendServer::onNewConnection()
{
    if (m_client) {
        m_client->disconnectFromHost();
        m_client->deleteLater();
    }
    m_client = m_server->nextPendingConnection();
    connect(m_client, &QTcpSocket::disconnected,
            this, &BackendServer::onClientDisconnected);
    connect(m_client, &QTcpSocket::readyRead,
            this, &BackendServer::onReadyRead);
    m_timer->start();
    emit clientConnected();
    qInfo() << "[Server] Android client connected from"
            << m_client->peerAddress().toString();
}

void BackendServer::onClientDisconnected()
{
    m_timer->stop();
    if (m_client) { m_client->deleteLater(); m_client = nullptr; }
    emit clientDisconnected();
    qInfo() << "[Server] Android client disconnected";
}

void BackendServer::onReadyRead()
{
    m_readBuf += m_client->readAll();
    while (true) {
        int idx = m_readBuf.indexOf('\n');
        if (idx < 0) break;
        QByteArray line = m_readBuf.left(idx).trimmed();
        m_readBuf.remove(0, idx + 1);
        if (line.isEmpty()) continue;
        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(line, &err);
        if (err.error == QJsonParseError::NoError)
            handleCommand(doc.object());
    }
}

void BackendServer::handleCommand(const QJsonObject& cmd)
{
    const QString type = cmd["type"].toString();
    if (type == "trigger") {
        const QString target = cmd["target"].toString("both");
        if      (target == "both") emit triggerRequested(0);
        else if (target == "cam1") emit triggerRequested(1);
        else if (target == "cam2") emit triggerRequested(2);
    } else if (type == "start") {
        emit startRequested();
    } else if (type == "stop") {
        emit stopRequested();
    } else if (type == "set") {
        emit paramChanged(cmd["param"].toString(), cmd["value"].toVariant());
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
    auto camJson = [](const CameraStatus& c) -> QJsonObject {
        return {{"frames",     c.frames    },
                {"fps",        c.fps       },
                {"written_gb", c.written_gb},
                {"bw_gbs",     c.bw_gbs    },
                {"dropped",    c.dropped   },
                {"buf_free",   c.buf_free  },
                {"geo_tagged", c.geo_tagged}};
    };

    auto gnss = std::atomic_load(&g_currentGnss);
    QJsonObject gnssObj;
    if (gnss && gnss->valid) {
        gnssObj = {{"valid",  true              },
                   {"fix",    gnss->fix_quality > 0
                               ? "GPS 3D" : "No fix"},
                   {"lat",    gnss->latitude    },
                   {"lon",    gnss->longitude   },
                   {"alt_m",  gnss->altitude_m  },
                   {"sats",   gnss->satellites  },
                   {"hdop",   gnss->hdop        },
                   {"utc",    gnss->utc_time    }};
    } else {
        gnssObj = {{"valid", false}};
    }

    return {{"type",  "status"          },
            {"cam1",  camJson(m_cam1)   },
            {"cam2",  camJson(m_cam2)   },
            {"gnss",  gnssObj           },
            {"sync",  QJsonObject{
                {"delta_ms",      0.8  },
                {"total_bw_gbs",  m_cam1.bw_gbs + m_cam2.bw_gbs },
                {"dropped_total", m_cam1.dropped + m_cam2.dropped}
            }}};
}

void BackendServer::updateCameraStatus(int camIndex, const CameraStatus& s)
{
    if      (camIndex == 1) m_cam1 = s;
    else if (camIndex == 2) m_cam2 = s;
}

void BackendServer::onFrameProcessed(int camIndex, bool geoTagged,
                                     int processMs, const QString& ts)
{
    if (!m_client) return;
    send({{"type",       "frame"    },
          {"cam",        camIndex   },
          {"geo_tagged", geoTagged  },
          {"process_ms", processMs  },
          {"ts",         ts         }});
}

void BackendServer::send(const QJsonObject& obj)
{
    if (!m_client) return;
    m_client->write(QJsonDocument(obj).toJson(QJsonDocument::Compact) + '\n');
}
