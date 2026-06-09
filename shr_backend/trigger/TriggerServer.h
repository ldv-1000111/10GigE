#pragma once
#include <QObject>
#include <QUdpSocket>

class TriggerServer : public QObject
{
    Q_OBJECT
public:
    explicit TriggerServer(quint16 port = 9001, QObject* parent = nullptr);
    bool start();
    void stop();

signals:
    void triggerReceived();
    void statusChanged(const QString& msg);

private slots:
    void onReadyRead();

private:
    QUdpSocket m_socket;
    quint16    m_port;
};
