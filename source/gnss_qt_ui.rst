Qt UI — GNSS Status Panel
==========================

The main window gains a GNSS status panel showing live fix data,
a port configuration widget, and a per-frame geo-tag indicator.

MainWindow Additions
---------------------

Add these members to ``MainWindow.h``:

.. code-block:: cpp

   // GNSS components
   NmeaReader*  m_nmeaReader  = nullptr;
   QThread*     m_gnssThread  = nullptr;

   // GNSS UI widgets
   QLabel*      m_gnssStatusLabel    = nullptr;
   QLabel*      m_gnssFixLabel       = nullptr;
   QLabel*      m_latLabel           = nullptr;
   QLabel*      m_lonLabel           = nullptr;
   QLabel*      m_altLabel           = nullptr;
   QLabel*      m_hdopLabel          = nullptr;
   QLabel*      m_satLabel           = nullptr;
   QComboBox*   m_portCombo          = nullptr;
   QComboBox*   m_baudCombo          = nullptr;
   QPushButton* m_gnssConnectBtn     = nullptr;
   QLabel*      m_geoTagIndicator    = nullptr;

GNSS Panel Layout
------------------

Build the GNSS status panel in the MainWindow constructor:

.. code-block:: cpp

   QGroupBox* gnssGroup = new QGroupBox("GNSS / GPS");
   QGridLayout* gnssLayout = new QGridLayout(gnssGroup);

   // Port selection row
   m_portCombo = new QComboBox();
   for (const auto& info : QSerialPortInfo::availablePorts())
       m_portCombo->addItem(info.portName());

   m_baudCombo = new QComboBox();
   for (int baud : {4800, 9600, 19200, 38400, 57600, 115200})
       m_baudCombo->addItem(QString::number(baud), baud);
   m_baudCombo->setCurrentText("9600");

   m_gnssConnectBtn = new QPushButton("Connect");

   gnssLayout->addWidget(new QLabel("Port:"), 0, 0);
   gnssLayout->addWidget(m_portCombo, 0, 1);
   gnssLayout->addWidget(new QLabel("Baud:"), 0, 2);
   gnssLayout->addWidget(m_baudCombo, 0, 3);
   gnssLayout->addWidget(m_gnssConnectBtn, 0, 4);

   // Status indicators
   m_gnssStatusLabel = new QLabel("Disconnected");
   m_gnssFixLabel    = new QLabel("No fix");
   m_gnssFixLabel->setStyleSheet("color: red; font-weight: bold;");

   gnssLayout->addWidget(new QLabel("Status:"), 1, 0);
   gnssLayout->addWidget(m_gnssStatusLabel, 1, 1, 1, 2);
   gnssLayout->addWidget(new QLabel("Fix:"), 1, 3);
   gnssLayout->addWidget(m_gnssFixLabel, 1, 4);

   // Coordinate display
   m_latLabel  = new QLabel("---.------°");
   m_lonLabel  = new QLabel("---.------°");
   m_altLabel  = new QLabel("--- m");
   m_hdopLabel = new QLabel("HDOP: --");
   m_satLabel  = new QLabel("Sats: --");

   gnssLayout->addWidget(new QLabel("Lat:"),  2, 0);
   gnssLayout->addWidget(m_latLabel,           2, 1);
   gnssLayout->addWidget(new QLabel("Lon:"),  2, 2);
   gnssLayout->addWidget(m_lonLabel,           2, 3);
   gnssLayout->addWidget(new QLabel("Alt:"),  2, 4);
   gnssLayout->addWidget(m_altLabel,           2, 5);
   gnssLayout->addWidget(m_hdopLabel,          3, 0, 1, 2);
   gnssLayout->addWidget(m_satLabel,           3, 2, 1, 2);

   // Geo-tag indicator
   m_geoTagIndicator = new QLabel("⬤ No GNSS");
   m_geoTagIndicator->setStyleSheet("color: gray;");
   gnssLayout->addWidget(m_geoTagIndicator, 3, 4, 1, 2);

   // Connect button signal
   connect(m_gnssConnectBtn, &QPushButton::clicked,
           this, &MainWindow::onGnssConnectClicked);

GNSS Slots
-----------

.. code-block:: cpp

   void MainWindow::onGnssConnectClicked()
   {
       if (!m_gnssThread)
       {
           m_gnssThread = new QThread(this);
           m_nmeaReader = new NmeaReader();
           m_nmeaReader->moveToThread(m_gnssThread);

           connect(m_gnssThread,  &QThread::started,
                   m_nmeaReader,  [this]() {
                       m_nmeaReader->open(
                           m_portCombo->currentText(),
                           m_baudCombo->currentData().toInt());
                   });

           connect(m_nmeaReader,  &NmeaReader::fixUpdated,
                   this,          &MainWindow::onGnssFixUpdated,
                   Qt::QueuedConnection);

           connect(m_nmeaReader,  &NmeaReader::statusChanged,
                   this,          &MainWindow::onGnssStatus,
                   Qt::QueuedConnection);

           connect(m_nmeaReader,  &NmeaReader::portError,
                   this,          &MainWindow::onGnssError,
                   Qt::QueuedConnection);

           m_gnssThread->start();
           m_gnssConnectBtn->setText("Disconnect");
       }
       else
       {
           QMetaObject::invokeMethod(m_nmeaReader, "close",
                                     Qt::QueuedConnection);
           m_gnssThread->quit();
           m_gnssThread->wait();
           delete m_nmeaReader;  m_nmeaReader = nullptr;
           delete m_gnssThread;  m_gnssThread = nullptr;
           m_gnssConnectBtn->setText("Connect");
           m_gnssStatusLabel->setText("Disconnected");
           m_gnssFixLabel->setText("No fix");
           m_gnssFixLabel->setStyleSheet("color: red; font-weight: bold;");
           m_geoTagIndicator->setText("⬤ No GNSS");
           m_geoTagIndicator->setStyleSheet("color: gray;");
       }
   }

   void MainWindow::onGnssFixUpdated(std::shared_ptr<GnssRecord> rec)
   {
       // This slot runs on the Qt main thread (QueuedConnection)
       if (!rec || !rec->valid) return;

       m_latLabel->setText(
           QString("%1°").arg(rec->latitude,  10, 'f', 6));
       m_lonLabel->setText(
           QString("%1°").arg(rec->longitude, 10, 'f', 6));
       m_altLabel->setText(
           QString("%1 m").arg(rec->altitude_m, 0, 'f', 1));
       m_hdopLabel->setText(
           QString("HDOP: %1").arg(rec->hdop, 0, 'f', 1));
       m_satLabel->setText(
           QString("Sats: %1").arg(rec->satellites));

       // Colour-code fix quality
       QString fixText;
       QString fixStyle;
       switch (rec->fix_quality)
       {
           case 0: fixText = "No fix";    fixStyle = "color: red;";    break;
           case 1: fixText = "GPS";       fixStyle = "color: green;";  break;
           case 2: fixText = "DGPS";      fixStyle = "color: green;";  break;
           case 4: fixText = "RTK Fixed"; fixStyle = "color: blue;";   break;
           case 5: fixText = "RTK Float"; fixStyle = "color: orange;"; break;
           default:fixText = "Fix";       fixStyle = "color: green;";  break;
       }
       fixStyle += " font-weight: bold;";
       m_gnssFixLabel->setText(fixText);
       m_gnssFixLabel->setStyleSheet(fixStyle);

       m_geoTagIndicator->setText("⬤ GNSS Active");
       m_geoTagIndicator->setStyleSheet("color: green; font-weight: bold;");
   }

   void MainWindow::onGnssStatus(const QString& status)
   {
       m_gnssStatusLabel->setText(status);
   }

   void MainWindow::onGnssError(const QString& error)
   {
       m_gnssStatusLabel->setText("Error: " + error);
       m_gnssStatusLabel->setStyleSheet("color: red;");
   }

Per-Frame Geo-Tag Indicator
-----------------------------

When the frame worker finishes processing a frame, it signals the UI.
Update the indicator to show whether that frame was geo-tagged:

.. code-block:: cpp

   // Connected to frameProcessed(int index, bool wasGeoTagged) signal:
   void MainWindow::onFrameProcessed(int index, bool geoTagged)
   {
       QString msg = QString("Frame %1: %2")
           .arg(index)
           .arg(geoTagged ? "✓ geo-tagged" : "⚠ no GNSS fix");
       m_frameStatusLabel->setText(msg);
   }
