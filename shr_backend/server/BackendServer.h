#pragma once
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QJsonObject>
#include "../gnss/GnssRecord.h"

class BackendServer : public QObject
{
    Q_OBJECT
public:
    struct CameraStatus {
        int    camIndex   = 0;
        int    frames     = 0;
        double fps        = 0.0;
        double written_gb = 0.0;
        double bw_gbs     = 0.0;
        int    dropped    = 0;
        int    buf_free   = 5;
        bool   geo_tagged = false;
    };

    explicit BackendServer(quint16 port = 9100, QObject* parent = nullptr);
    bool start();
    void stop();
    void updateCameraStatus(int camIndex, const CameraStatus& s);

signals:
    void triggerRequested(int camIndex);   // 0=both,1=cam1,2=cam2
    void startRequested();
    void stopRequested();
    void paramChanged(const QString& param, const QVariant& value);
    void clientConnected();
    void clientDisconnected();

public slots:
    void onFrameProcessed(int camIndex, bool geoTagged,
                          int processMs, const QString& ts);

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onReadyRead();
    void pushStatus();

private:
    void        handleCommand(const QJsonObject& cmd);
    void        send(const QJsonObject& obj);
    QJsonObject buildStatusPayload() const;

    QTcpServer*  m_server   = nullptr;
    QTcpSocket*  m_client   = nullptr;
    QTimer*      m_timer    = nullptr;
    quint16      m_port;
    QByteArray   m_readBuf;

    CameraStatus m_cam1, m_cam2;
};
