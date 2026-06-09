#include "TriggerServer.h"
#include <QNetworkDatagram>
#include <QDebug>

TriggerServer::TriggerServer(quint16 port, QObject* parent)
    : QObject(parent), m_port(port)
{
    connect(&m_socket, &QUdpSocket::readyRead,
            this, &TriggerServer::onReadyRead);
}

bool TriggerServer::start()
{
    if (!m_socket.bind(QHostAddress::AnyIPv4, m_port)) {
        emit statusChanged("TriggerServer: cannot bind UDP :" + QString::number(m_port));
        return false;
    }
    emit statusChanged("TriggerServer: listening on UDP :" + QString::number(m_port));
    qInfo() << "[Trigger] UDP server listening on :" << m_port;
    return true;
}

void TriggerServer::stop() { m_socket.close(); }

void TriggerServer::onReadyRead()
{
    while (m_socket.hasPendingDatagrams()) {
        QNetworkDatagram dg = m_socket.receiveDatagram();
        QString msg = QString::fromUtf8(dg.data()).trimmed();
        if (msg.toLower().contains("trigger")) {
            qInfo() << "[Trigger] Received from" << dg.senderAddress().toString();
            emit triggerReceived();
        }
    }
}
