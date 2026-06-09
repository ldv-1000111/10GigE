#include "NmeaReader.h"
#include <QDebug>

std::shared_ptr<GnssRecord> g_currentGnss = std::make_shared<GnssRecord>();

NmeaReader::NmeaReader(QObject* parent) : QObject(parent)
{
    connect(&m_serial, &QSerialPort::readyRead,
            this, &NmeaReader::onReadyRead);
    connect(&m_serial, &QSerialPort::errorOccurred,
            this, &NmeaReader::onSerialError);
}

NmeaReader::~NmeaReader() { close(); }

void NmeaReader::open(const QString& portName, int baudRate)
{
    m_serial.setPortName(portName);
    m_serial.setBaudRate(baudRate);
    m_serial.setDataBits(QSerialPort::Data8);
    m_serial.setParity(QSerialPort::NoParity);
    m_serial.setStopBits(QSerialPort::OneStop);
    m_serial.setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serial.open(QIODevice::ReadOnly)) {
        emit portError("Cannot open " + portName + ": " + m_serial.errorString());
        return;
    }
    emit statusChanged("Open: " + portName + " @ " + QString::number(baudRate));
    qInfo() << "[GNSS] Opened" << portName << "at" << baudRate << "baud";
}

void NmeaReader::close()
{
    if (m_serial.isOpen()) m_serial.close();
    emit statusChanged("Closed");
}

void NmeaReader::onReadyRead()
{
    m_buffer += m_serial.readAll();
    while (true) {
        int end = m_buffer.indexOf('\n');
        if (end < 0) break;
        QString sentence = QString::fromLatin1(m_buffer.left(end)).trimmed();
        m_buffer.remove(0, end + 1);
        if (sentence.startsWith('$')) processSentence(sentence);
    }
}

void NmeaReader::processSentence(const QString& sentence)
{
    if (!validateChecksum(sentence)) return;

    QString body = sentence.section('*', 0, 0);
    QStringList fields = body.split(',');
    if (fields.isEmpty()) return;

    QString type = fields[0].mid(1); // strip $
    bool updated = false;

    if (type.endsWith("GGA") && fields.size() >= 15) {
        m_current.raw_gga = sentence;
        updated = parseGGA(fields, m_current);
    } else if (type.endsWith("RMC") && fields.size() >= 12) {
        m_current.raw_rmc = sentence;
        updated = parseRMC(fields, m_current);
    }

    if (updated) {
        m_current.host_timestamp = QDateTime::currentDateTimeUtc();
        auto record = std::make_shared<GnssRecord>(m_current);
        std::atomic_store(&g_currentGnss, record);
        emit fixUpdated(record);
    }
}

bool NmeaReader::parseGGA(const QStringList& f, GnssRecord& rec)
{
    rec.utc_time    = f[1];
    rec.fix_quality = f[6].toInt();
    if (rec.fix_quality == 0) { rec.valid = false; return true; }
    rec.latitude   = nmeaToDecimal(f[2], f[3]);
    rec.longitude  = nmeaToDecimal(f[4], f[5]);
    rec.satellites = f[7].toInt();
    rec.hdop       = f[8].toDouble();
    rec.altitude_m = f[9].toDouble();
    rec.valid      = true;
    return true;
}

bool NmeaReader::parseRMC(const QStringList& f, GnssRecord& rec)
{
    if (f[2] != "A") return false;
    rec.utc_time  = f[1];
    rec.utc_date  = f[9];
    rec.speed_kn  = f[7].toDouble();
    rec.track_deg = f[8].toDouble();
    return true;
}

bool NmeaReader::validateChecksum(const QString& sentence)
{
    int dol  = sentence.indexOf('$');
    int star = sentence.lastIndexOf('*');
    if (dol < 0 || star < 0 || star < dol + 2) return false;
    uint8_t computed = 0;
    for (int i = dol + 1; i < star; ++i)
        computed ^= static_cast<uint8_t>(sentence[i].toLatin1());
    bool ok = false;
    uint8_t declared = sentence.mid(star + 1, 2).toUInt(&ok, 16);
    return ok && (computed == declared);
}

double NmeaReader::nmeaToDecimal(const QString& value, const QString& hemi)
{
    if (value.isEmpty()) return 0.0;
    bool ok = false;
    double raw = value.toDouble(&ok);
    if (!ok) return 0.0;
    int    deg = static_cast<int>(raw / 100);
    double min = raw - (deg * 100.0);
    double dec = deg + (min / 60.0);
    if (hemi == "S" || hemi == "W") dec = -dec;
    return dec;
}

void NmeaReader::onSerialError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError)
        emit portError("Serial error: " + m_serial.errorString());
}
