NMEA Serial Reader Implementation
===================================

The ``NmeaReader`` class runs on a dedicated ``QThread``, owns a
``QSerialPort``, and continuously parses incoming NMEA sentences. It
emits a Qt signal when a new valid fix is available and atomically
updates the shared ``GnssRecord``.

NmeaReader.h
-------------

.. code-block:: cpp

   #pragma once
   #include <QObject>
   #include <QSerialPort>
   #include <QDateTime>
   #include <memory>
   #include <atomic>

   struct GnssRecord
   {
       double    latitude    = 0.0;
       double    longitude   = 0.0;
       double    altitude_m  = 0.0;
       double    speed_kn    = 0.0;
       double    track_deg   = 0.0;
       double    hdop        = 99.9;
       int       satellites  = 0;
       int       fix_quality = 0;
       QString   utc_time;
       QString   utc_date;
       QString   raw_gga;
       QString   raw_rmc;
       QDateTime host_timestamp;
       bool      valid = false;
   };

   // Atomic shared pointer — written by NmeaReader, read by FrameObserver
   extern std::shared_ptr<GnssRecord> g_currentGnss;

   class NmeaReader : public QObject
   {
       Q_OBJECT
   public:
       explicit NmeaReader(QObject* parent = nullptr);
       ~NmeaReader();

   public slots:
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
       bool        parseGGA(const QStringList& fields, GnssRecord& rec);
       bool        parseRMC(const QStringList& fields, GnssRecord& rec);
       bool        validateChecksum(const QString& sentence);
       double      nmeaToDecimal(const QString& value, const QString& hemi);
       void        processSentence(const QString& sentence);

       QSerialPort m_serial;
       QByteArray  m_buffer;
       GnssRecord  m_current;    // working copy, updated per sentence
   };

NmeaReader.cpp
---------------

.. code-block:: cpp

   #include "NmeaReader.h"
   #include <QDebug>

   // Global atomic shared pointer — initialised to empty record
   std::shared_ptr<GnssRecord> g_currentGnss =
       std::make_shared<GnssRecord>();

   NmeaReader::NmeaReader(QObject* parent)
       : QObject(parent)
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

       if (!m_serial.open(QIODevice::ReadOnly))
       {
           emit portError("Cannot open " + portName + ": " +
                          m_serial.errorString());
           return;
       }
       emit statusChanged("Open: " + portName +
                          " @ " + QString::number(baudRate));
   }

   void NmeaReader::close()
   {
       if (m_serial.isOpen())
           m_serial.close();
       emit statusChanged("Closed");
   }

   void NmeaReader::onReadyRead()
   {
       m_buffer += m_serial.readAll();

       // Extract complete sentences ending with \r\n or \n
       while (true)
       {
           int end = m_buffer.indexOf('\n');
           if (end < 0) break;

           QString sentence = QString::fromLatin1(
               m_buffer.left(end)).trimmed();
           m_buffer.remove(0, end + 1);

           if (sentence.startsWith('$'))
               processSentence(sentence);
       }
   }

   void NmeaReader::processSentence(const QString& sentence)
   {
       if (!validateChecksum(sentence)) return;

       // Strip checksum for field parsing
       QString body = sentence.section('*', 0, 0);
       QStringList fields = body.split(',');
       if (fields.isEmpty()) return;

       QString type = fields[0].mid(1);   // e.g. "GNGGA", "GNRMC"

       bool updated = false;

       if (type.endsWith("GGA") && fields.size() >= 15)
       {
           m_current.raw_gga = sentence;
           updated = parseGGA(fields, m_current);
       }
       else if (type.endsWith("RMC") && fields.size() >= 12)
       {
           m_current.raw_rmc = sentence;
           updated = parseRMC(fields, m_current);
       }

       if (updated && m_current.valid)
       {
           m_current.host_timestamp = QDateTime::currentDateTimeUtc();
           auto record = std::make_shared<GnssRecord>(m_current);
           std::atomic_store(&g_currentGnss, record);
           emit fixUpdated(record);
       }
   }

   bool NmeaReader::parseGGA(const QStringList& f, GnssRecord& rec)
   {
       // Field indices per NMEA GGA spec
       rec.utc_time    = f[1];
       rec.fix_quality = f[6].toInt();

       if (rec.fix_quality == 0) return false;   // no fix

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
       if (f[2] != "A") return false;   // void — no valid fix

       rec.utc_time  = f[1];
       rec.utc_date  = f[9];
       rec.speed_kn  = f[7].toDouble();
       rec.track_deg = f[8].toDouble();
       // Don't overwrite position — GGA has altitude, RMC doesn't
       return true;
   }

   bool NmeaReader::validateChecksum(const QString& sentence)
   {
       int dollarPos = sentence.indexOf('$');
       int starPos   = sentence.lastIndexOf('*');
       if (dollarPos < 0 || starPos < 0 || starPos < dollarPos + 2)
           return false;

       uint8_t computed = 0;
       for (int i = dollarPos + 1; i < starPos; ++i)
           computed ^= static_cast<uint8_t>(sentence[i].toLatin1());

       bool ok = false;
       uint8_t declared = sentence.mid(starPos + 1, 2).toUInt(&ok, 16);
       return ok && (computed == declared);
   }

   double NmeaReader::nmeaToDecimal(const QString& value,
                                    const QString& hemi)
   {
       if (value.isEmpty()) return 0.0;
       bool ok = false;
       double raw = value.toDouble(&ok);
       if (!ok) return 0.0;

       int    degrees = static_cast<int>(raw / 100);
       double minutes = raw - (degrees * 100.0);
       double decimal = degrees + (minutes / 60.0);

       if (hemi == "S" || hemi == "W") decimal = -decimal;
       return decimal;
   }

   void NmeaReader::onSerialError(QSerialPort::SerialPortError error)
   {
       if (error != QSerialPort::NoError)
           emit portError("Serial error: " + m_serial.errorString());
   }

Starting the Reader on Its Own Thread
---------------------------------------

The ``NmeaReader`` must live on a ``QThread`` so that its ``QSerialPort``
event loop runs independently of the Qt main thread:

.. code-block:: cpp

   // In MainWindow constructor or application startup:
   m_gnssThread = new QThread(this);
   m_nmeaReader = new NmeaReader();         // no parent — will be moved
   m_nmeaReader->moveToThread(m_gnssThread);

   connect(m_gnssThread, &QThread::started,
           m_nmeaReader, [this]() {
               m_nmeaReader->open("/dev/ttyUSB0", 9600);
           });

   connect(m_nmeaReader, &NmeaReader::fixUpdated,
           this, &MainWindow::onGnssFixUpdated,
           Qt::QueuedConnection);           // cross-thread signal

   connect(m_nmeaReader, &NmeaReader::portError,
           this, &MainWindow::onGnssError,
           Qt::QueuedConnection);

   m_gnssThread->start();
