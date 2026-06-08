Option 3 — Software Trigger via Qt Application
================================================

When the external computer sends a network message to the **Bedrock V3000
Qt application**, the app relays it as a ``TriggerSoftware`` command to
the camera via Vimba X. This is the simplest trigger path to implement
but introduces ~1–10 ms of software latency.

.. code-block:: text

   External Computer                V3000 Qt App            SHR Camera
   ─────────────────                ────────────            ──────────
   TCP/UDP/HTTP/MQTT ──────────►  Qt slot receives  ──►  TriggerSoftware
   "trigger now"                   message               RunCommand()
   message                              │                      │
                                   (1–10 ms latency)          ▼
                                                        Frame in stream

When to Use This Method
------------------------

- The external computer is a PC, server, or embedded device that can
  open a TCP/UDP socket
- Trigger timing precision of ~1–10 ms is acceptable
- You want to avoid additional wiring
- The trigger logic is already in software on the external computer

When **not** to use this method:

- Sub-millisecond timing precision is required (use Option 2 instead)
- Multi-camera synchronisation is needed (use Option 1 instead)
- The external computer cannot reach the V3000 on the network

Part A — Qt Trigger Server (V3000 Side)
-----------------------------------------

Add a ``TriggerServer`` class to the Qt application that listens for
incoming trigger messages:

.. code-block:: cpp

   // TriggerServer.h
   #pragma once
   #include <QObject>
   #include <QUdpSocket>

   class TriggerServer : public QObject
   {
       Q_OBJECT
   public:
       explicit TriggerServer(quint16 port = 9001,
                              QObject* parent = nullptr);
       bool start();
       void stop();

   signals:
       void triggerReceived();      // connected to camera trigger slot
       void statusChanged(const QString& msg);

   private slots:
       void onReadyRead();

   private:
       QUdpSocket m_socket;
       quint16    m_port;
   };

.. code-block:: cpp

   // TriggerServer.cpp
   #include "TriggerServer.h"
   #include <QNetworkDatagram>

   TriggerServer::TriggerServer(quint16 port, QObject* parent)
       : QObject(parent), m_port(port)
   {
       connect(&m_socket, &QUdpSocket::readyRead,
               this, &TriggerServer::onReadyRead);
   }

   bool TriggerServer::start()
   {
       if (!m_socket.bind(QHostAddress::AnyIPv4, m_port))
       {
           emit statusChanged(
               "Cannot bind UDP port " +
               QString::number(m_port) + ": " +
               m_socket.errorString());
           return false;
       }
       emit statusChanged(
           "Listening on UDP port " + QString::number(m_port));
       return true;
   }

   void TriggerServer::stop()
   {
       m_socket.close();
   }

   void TriggerServer::onReadyRead()
   {
       while (m_socket.hasPendingDatagrams())
       {
           QNetworkDatagram dg = m_socket.receiveDatagram();
           QString msg = QString::fromUtf8(dg.data()).trimmed();

           // Accept any message containing "trigger" (case-insensitive)
           // Extend with JSON parsing for richer commands
           if (msg.toLower().contains("trigger"))
               emit triggerReceived();
       }
   }

Part B — Fire the Camera on Trigger Signal
-------------------------------------------

Connect ``TriggerServer::triggerReceived`` to a slot that issues
``TriggerSoftware`` via Vimba X:

.. code-block:: cpp

   // In MainWindow.cpp — during camera setup:

   // 1. Configure the camera for software trigger mode
   setEnum(m_camera, "TriggerSelector",   "FrameStart");
   setEnum(m_camera, "TriggerSource",     "Software");
   setEnum(m_camera, "TriggerActivation", "RisingEdge");
   setEnum(m_camera, "TriggerMode",       "On");
   runCmd (m_camera, "AcquisitionStart");

   // 2. Start the trigger server
   m_triggerServer = new TriggerServer(9001, this);
   connect(m_triggerServer, &TriggerServer::triggerReceived,
           this,            &MainWindow::onExternalTrigger,
           Qt::QueuedConnection);
   m_triggerServer->start();

.. code-block:: cpp

   // The trigger slot — fires the camera sensor
   void MainWindow::onExternalTrigger()
   {
       if (!m_camera) return;

       FeaturePtr feature;
       if (VmbErrorSuccess ==
           m_camera->GetFeatureByName("TriggerSoftware", feature))
       {
           feature->RunCommand();
           // RunCommand() is non-blocking — returns in microseconds
           // The resulting frame arrives in FrameReceived() asynchronously
       }
   }

Part C — Send the Trigger (External Computer)
----------------------------------------------

The external computer sends a simple UDP datagram to the V3000's IP
on port 9001. Any language or tool works:

**From the command line:**

.. code-block:: bash

   # Send a trigger from any Linux/macOS machine
   echo "trigger" | nc -u 192.168.10.1 9001

   # Or with Python one-liner:
   python3 -c "import socket; s=socket.socket(socket.AF_INET, \
       socket.SOCK_DGRAM); s.sendto(b'trigger', ('192.168.10.1', 9001))"

**Python client (external computer):**

.. code-block:: python

   import socket
   import time

   V3000_IP   = "192.168.10.1"
   TRIGGER_PORT = 9001

   sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

   # Trigger 5 frames at 1-second intervals
   for i in range(5):
       sock.sendto(b"trigger", (V3000_IP, TRIGGER_PORT))
       print(f"Trigger {i+1} sent")
       time.sleep(1.0)

   sock.close()

**Qt client on the external computer:**

.. code-block:: cpp

   #include <QUdpSocket>
   #include <QHostAddress>

   class TriggerClient : public QObject
   {
       Q_OBJECT
   public:
       TriggerClient(const QString& host, quint16 port, QObject* p=nullptr)
           : QObject(p), m_host(host), m_port(port) {}

   public slots:
       void sendTrigger()
       {
           m_socket.writeDatagram(
               "trigger",
               QHostAddress(m_host),
               m_port);
       }

   private:
       QUdpSocket m_socket;
       QString    m_host;
       quint16    m_port;
   };

   // Connect a QPushButton or QTimer to TriggerClient::sendTrigger()

Extending the Protocol
-----------------------

The simple ``"trigger"`` string message can be extended to carry
additional data. For example, a JSON payload allows the external computer
to pass context that the Qt app can include in the frame metadata:

.. code-block:: json

   {
     "command":    "trigger",
     "sequence_id": 42,
     "label":      "inspection_point_7",
     "requested_at_utc": "2026-06-07T14:23:11.000Z"
   }

The Qt app parses this with ``QJsonDocument`` and can include the
``label`` and ``sequence_id`` in the JSON sidecar file alongside the
GNSS data, creating a fully annotated frame record.

Latency Characteristics
------------------------

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Component
     - Typical latency
   * - UDP packet transmission
     - < 1 ms (LAN)
   * - Qt event loop processing
     - 0.5–5 ms (depends on UI load)
   * - ``TriggerSoftware RunCommand()``
     - < 1 ms
   * - Camera processing + exposure start
     - 1–5 ms (firmware dependent)
   * - **Total end-to-end**
     - **~3–12 ms**

For applications where this latency is unacceptable, switch to
:doc:`trigger_action_commands` (microseconds) or
:doc:`trigger_hardware_ttl` (nanoseconds).
