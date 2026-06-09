#pragma once
#include <QObject>
#include <QSerialPort>
#include <QLocalSocket>
#include <memory>
#include "GnssRecord.h"

class NmeaReader : public QObject
{
    Q_OBJECT
public:
    explicit NmeaReader(QObject* parent = nullptr);
    ~NmeaReader();

public slots:
    // portName: /dev/ttyUSB0  OR  /tmp/ttyGNSS (socat virtual)
    void open(const QString& portName, int baudRate = 9600);
    void close();

signals:
    void fixUpdated(std::shared_ptr<GnssRecord> record);
    void portError(const QString& error);
    void statusChanged(const QString& status);

private slots:
    void onReadyRead();
    void onSerialError(QSerialPort::SerialPortError error);

private:
    bool   parseGGA(const QStringList& fields, GnssRecord& rec);
    bool   parseRMC(const QStringList& fields, GnssRecord& rec);
    bool   validateChecksum(const QString& sentence);
    double nmeaToDecimal(const QString& value, const QString& hemi);
    void   processSentence(const QString& sentence);

    QSerialPort m_serial;
    QByteArray  m_buffer;
    GnssRecord  m_current;
};
