Qt Application Layout & File Structure
========================================

This chapter describes how the Qt application is structured — which files
own which parts of the UI, and how the widgets map to the visual layout
shown in the mockup. The same structure supports both single-camera and
dual-camera configurations by reusing ``CameraPanel`` across both cases.

File Structure
--------------

.. code-block:: text

   shr_camera_app/
   ├── CMakeLists.txt
   ├── main.cpp
   │
   ├── MainWindow.h              ← master layout, menu, status bar
   ├── MainWindow.cpp
   │
   ├── camera/
   │   ├── CameraPanel.h         ← single camera preview + metrics bar
   │   ├── CameraPanel.cpp         instantiated once per camera
   │   ├── CameraWorker.h        ← Vimba X acquisition thread per camera
   │   └── CameraWorker.cpp
   │
   ├── gnss/
   │   ├── GnssPanel.h           ← GNSS status section (right panel)
   │   ├── GnssPanel.cpp
   │   ├── NmeaReader.h          ← QSerialPort + NMEA parser (see Part VI)
   │   └── NmeaReader.cpp
   │
   ├── trigger/
   │   ├── TriggerPanel.h        ← trigger section in left sidebar
   │   ├── TriggerPanel.cpp
   │   ├── TriggerServer.h       ← UDP listener for software trigger
   │   └── TriggerServer.cpp
   │
   ├── sync/
   │   ├── SyncStatusPanel.h     ← dual-camera sync metrics (right panel)
   │   └── SyncStatusPanel.cpp
   │
   └── log/
       ├── FrameLogPanel.h       ← interleaved per-camera frame log
       └── FrameLogPanel.cpp

Top-Level Layout — MainWindow
------------------------------

The root layout is a horizontal split of three columns: left sidebar,
main preview area, and right panel. A ``QStatusBar`` runs across the
bottom.

.. code-block:: cpp

   // MainWindow.h
   #pragma once
   #include <QMainWindow>

   class CameraPanel;
   class GnssPanel;
   class SyncStatusPanel;
   class FrameLogPanel;
   class TriggerPanel;

   class MainWindow : public QMainWindow
   {
       Q_OBJECT
   public:
       explicit MainWindow(QWidget* parent = nullptr);

   private:
       QWidget* buildSidebar();
       QWidget* buildMainArea();
       QWidget* buildRightPanel();
       void     buildStatusBar();
       void     buildConnections();

       // Camera panels — one per camera
       CameraPanel*      m_camPanel1  = nullptr;
       CameraPanel*      m_camPanel2  = nullptr;   // nullptr for single-camera

       // Right panel widgets
       GnssPanel*        m_gnssPanel  = nullptr;
       SyncStatusPanel*  m_syncPanel  = nullptr;
       FrameLogPanel*    m_logPanel   = nullptr;

       // Sidebar panels
       TriggerPanel*     m_trigPanel  = nullptr;
   };

.. code-block:: cpp

   // MainWindow.cpp — central widget and root layout
   #include "MainWindow.h"
   #include "camera/CameraPanel.h"
   #include "gnss/GnssPanel.h"
   #include "sync/SyncStatusPanel.h"
   #include "log/FrameLogPanel.h"
   #include "trigger/TriggerPanel.h"
   #include <QHBoxLayout>
   #include <QVBoxLayout>
   #include <QStatusBar>
   #include <QLabel>

   MainWindow::MainWindow(QWidget* parent)
       : QMainWindow(parent)
   {
       setWindowTitle("SHR Dual Camera Control");
       resize(1280, 800);

       QWidget* central = new QWidget(this);
       setCentralWidget(central);

       QHBoxLayout* root = new QHBoxLayout(central);
       root->setContentsMargins(0, 0, 0, 0);
       root->setSpacing(0);

       // Three-column split
       QWidget* sidebar    = buildSidebar();
       QWidget* mainArea   = buildMainArea();
       QWidget* rightPanel = buildRightPanel();

       sidebar->setFixedWidth(240);
       rightPanel->setFixedWidth(200);

       root->addWidget(sidebar);
       root->addWidget(mainArea, 1);   // stretch: fills remaining space
       root->addWidget(rightPanel);

       buildStatusBar();
       buildConnections();
   }

Left Sidebar — buildSidebar()
------------------------------

The sidebar stacks panels vertically, each separated by a 0.5px border.
A ``QScrollArea`` wraps the content for smaller screen heights.

.. code-block:: cpp

   QWidget* MainWindow::buildSidebar()
   {
       QScrollArea* scroll = new QScrollArea;
       scroll->setWidgetResizable(true);
       scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
       scroll->setFrameShape(QFrame::NoFrame);

       QWidget*     content = new QWidget;
       QVBoxLayout* layout  = new QVBoxLayout(content);
       layout->setContentsMargins(0, 0, 0, 0);
       layout->setSpacing(0);

       // Shared camera settings (pixel format, exposure, gain, debayer)
       layout->addWidget(buildSharedSettingsSection());

       // Per-camera info blocks
       layout->addWidget(buildCameraInfoSection(1, "192.168.10.41"));
       layout->addWidget(buildCameraInfoSection(2, "192.168.10.42"));

       // Trigger control
       m_trigPanel = new TriggerPanel(this);
       layout->addWidget(m_trigPanel);

       // Acquisition control (buffers, output dir, start/stop)
       layout->addWidget(buildAcquisitionSection());

       layout->addStretch();   // push all sections to top
       scroll->setWidget(content);
       return scroll;
   }

Main Preview Area — buildMainArea()
-------------------------------------

Two ``CameraPanel`` instances stacked vertically with equal stretch,
each identified by a colour accent passed at construction.

.. code-block:: cpp

   QWidget* MainWindow::buildMainArea()
   {
       QWidget*     w      = new QWidget;
       QVBoxLayout* layout = new QVBoxLayout(w);
       layout->setContentsMargins(0, 0, 0, 0);
       layout->setSpacing(0);

       m_camPanel1 = new CameraPanel("Camera 1",
                                     QColor("#378ADD"),   // blue accent
                                     "192.168.10.41",
                                     "SFP+ 0", this);

       m_camPanel2 = new CameraPanel("Camera 2",
                                     QColor("#1D9E75"),   // teal accent
                                     "192.168.10.42",
                                     "SFP+ 1", this);

       layout->addWidget(m_camPanel1, 1);   // equal stretch — 50 / 50
       layout->addWidget(m_camPanel2, 1);
       return w;
   }

.. note::
   For a single-camera build, omit ``m_camPanel2`` and use
   ``layout->addWidget(m_camPanel1, 1)`` with no second entry.
   ``CameraPanel`` is self-contained and works identically with one
   instance.

Right Panel — buildRightPanel()
---------------------------------

Three stacked sections: GNSS (fixed height), sync status (fixed height),
and frame log (stretches to fill remaining space).

.. code-block:: cpp

   QWidget* MainWindow::buildRightPanel()
   {
       QWidget*     w      = new QWidget;
       QVBoxLayout* layout = new QVBoxLayout(w);
       layout->setContentsMargins(0, 0, 0, 0);
       layout->setSpacing(0);

       m_gnssPanel = new GnssPanel(this);
       m_syncPanel = new SyncStatusPanel(this);
       m_logPanel  = new FrameLogPanel(this);

       layout->addWidget(m_gnssPanel);           // fixed height
       layout->addWidget(m_syncPanel);           // fixed height
       layout->addWidget(m_logPanel, 1);         // stretch: fills rest
       return w;
   }

Status Bar — buildStatusBar()
-------------------------------

Qt's built-in ``QStatusBar`` carries permanent network and SDK info.
Left-aligned labels for each camera; right-aligned for SDK versions.

.. code-block:: cpp

   void MainWindow::buildStatusBar()
   {
       auto* cam1Status = new QLabel(
           "● Cam 1: 192.168.10.41 · SFP+0 · MTU 9000");
       auto* cam2Status = new QLabel(
           "● Cam 2: 192.168.10.42 · SFP+1 · MTU 9000");
       auto* sdkStatus  = new QLabel(
           "Vimba X 2026-1  ·  Qt 6.8 LTS");

       // Colour the camera dots to match panel accents
       cam1Status->setStyleSheet("color: #378ADD;");
       cam2Status->setStyleSheet("color: #1D9E75;");
       sdkStatus->setStyleSheet("color: gray; font-size: 11px;");

       statusBar()->addWidget(cam1Status);
       statusBar()->addWidget(cam2Status);
       statusBar()->addPermanentWidget(sdkStatus);  // right-aligned
   }

CameraPanel — the Reusable Camera Widget
-----------------------------------------

``CameraPanel`` is the core reusable widget. It owns the preview
thumbnail, the camera header bar, and the per-camera metrics row.
It is completely self-contained — it knows nothing about the other
camera.

.. code-block:: cpp

   // camera/CameraPanel.h
   #pragma once
   #include <QWidget>
   #include <QLabel>
   #include <QColor>

   class CameraPanel : public QWidget
   {
       Q_OBJECT
   public:
       CameraPanel(const QString& name,
                   const QColor&  accentColor,
                   const QString& ipAddress,
                   const QString& portLabel,
                   QWidget*       parent = nullptr);

   public slots:
       // Called from CameraWorker via Qt::QueuedConnection
       void onFrameReceived(int           frameIndex,
                            const QImage& thumbnail,
                            bool          geoTagged);

       void updateMetrics(double  fps,
                          qint64  bytesWritten,
                          int     buffersFree,
                          double  bandwidthGBs);

   private:
       void buildLayout();
       void applyAccentColor();

       // Header bar
       QLabel*  m_headerLabel   = nullptr;
       QLabel*  m_portBadge     = nullptr;

       // Preview
       QLabel*  m_previewLabel  = nullptr;   // displays QPixmap thumbnail

       // Overlays (painted as child widgets over the preview)
       QLabel*  m_overlayTL     = nullptr;   // frame index, resolution, geo-tag
       QLabel*  m_overlayTR     = nullptr;   // downsample ratio, pixel format

       // Metrics bar
       QLabel*  m_framesLabel   = nullptr;
       QLabel*  m_fpsLabel      = nullptr;
       QLabel*  m_writtenLabel  = nullptr;
       QLabel*  m_bufferLabel   = nullptr;
       QLabel*  m_bwLabel       = nullptr;

       QString  m_name;
       QColor   m_accentColor;
   };

.. code-block:: cpp

   // camera/CameraPanel.cpp — key methods
   #include "CameraPanel.h"
   #include <QVBoxLayout>
   #include <QHBoxLayout>
   #include <QPixmap>

   CameraPanel::CameraPanel(const QString& name,
                             const QColor&  accentColor,
                             const QString& ipAddress,
                             const QString& portLabel,
                             QWidget*       parent)
       : QWidget(parent)
       , m_name(name)
       , m_accentColor(accentColor)
   {
       buildLayout();
       m_headerLabel->setText(
           name + "  —  " + ipAddress);
       m_portBadge->setText(portLabel);
       applyAccentColor();
   }

   void CameraPanel::onFrameReceived(int           index,
                                     const QImage& thumbnail,
                                     bool          geoTagged)
   {
       // Update preview — must be called on Qt main thread
       m_previewLabel->setPixmap(
           QPixmap::fromImage(thumbnail).scaled(
               m_previewLabel->size(),
               Qt::KeepAspectRatio,
               Qt::FastTransformation));

       // Update overlay
       m_overlayTL->setText(
           QString("Frame #%1\n%2\n%3")
               .arg(index)
               .arg("14192 × 10640")
               .arg(geoTagged ? "⬤ geo-tagged" : "⚠ no GNSS fix"));
   }

   void CameraPanel::updateMetrics(double  fps,
                                   qint64  bytesWritten,
                                   int     buffersFree,
                                   double  bandwidthGBs)
   {
       m_fpsLabel    ->setText(QString("%1 fps").arg(fps, 0, 'f', 1));
       m_writtenLabel->setText(
           QString("%1 GB").arg(bytesWritten / 1e9, 0, 'f', 0));
       m_bufferLabel ->setText(
           QString("buf %1/5").arg(buffersFree));
       m_bwLabel     ->setText(
           QString("%1 GB/s").arg(bandwidthGBs, 0, 'f', 2));
   }

CameraWorker — Acquisition Thread
-----------------------------------

Each camera gets its own ``CameraWorker`` that lives on a ``QThread``.
It owns the Vimba X ``CameraPtr`` and runs the ``FrameObserver``.
It communicates back to ``CameraPanel`` exclusively via Qt signals.

.. code-block:: cpp

   // camera/CameraWorker.h
   #pragma once
   #include <QObject>
   #include <QThread>
   #include <QImage>
   #include <memory>
   #include "VmbCPP/VmbCPP.h"

   class CameraWorker : public QObject
   {
       Q_OBJECT
   public:
       explicit CameraWorker(const QString& cameraIP,
                             QObject*       parent = nullptr);

   public slots:
       void start();
       void stop();

   signals:
       // All signals are Qt::QueuedConnection to the main thread
       void frameReady(int frameIndex, QImage thumbnail, bool geoTagged);
       void metricsUpdated(double fps, qint64 written,
                           int bufFree, double bwGBs);
       void errorOccurred(const QString& msg);
       void statusChanged(const QString& msg);

   private:
       QString              m_cameraIP;
       VmbCPP::CameraPtr    m_camera;
       // ... Vimba X frame buffers, observer, etc.
   };

Connecting Workers to Panels
------------------------------

In ``MainWindow::buildConnections()``, each worker is connected to its
corresponding panel via ``Qt::QueuedConnection`` — the worker runs on a
background thread, the panel updates on the main thread:

.. code-block:: cpp

   void MainWindow::buildConnections()
   {
       // Worker threads
       auto* thread1 = new QThread(this);
       auto* thread2 = new QThread(this);

       auto* worker1 = new CameraWorker("192.168.10.41");
       auto* worker2 = new CameraWorker("192.168.10.42");

       worker1->moveToThread(thread1);
       worker2->moveToThread(thread2);

       // Wire frames and metrics to their panels
       connect(worker1, &CameraWorker::frameReady,
               m_camPanel1, &CameraPanel::onFrameReceived,
               Qt::QueuedConnection);

       connect(worker1, &CameraWorker::metricsUpdated,
               m_camPanel1, &CameraPanel::updateMetrics,
               Qt::QueuedConnection);

       connect(worker2, &CameraWorker::frameReady,
               m_camPanel2, &CameraPanel::onFrameReceived,
               Qt::QueuedConnection);

       connect(worker2, &CameraWorker::metricsUpdated,
               m_camPanel2, &CameraPanel::updateMetrics,
               Qt::QueuedConnection);

       // Wire frame log — both workers feed the same log panel
       connect(worker1, &CameraWorker::frameReady,
               m_logPanel, &FrameLogPanel::onFrameReady,
               Qt::QueuedConnection);

       connect(worker2, &CameraWorker::frameReady,
               m_logPanel, &FrameLogPanel::onFrameReady,
               Qt::QueuedConnection);

       // Start on thread start
       connect(thread1, &QThread::started, worker1, &CameraWorker::start);
       connect(thread2, &QThread::started, worker2, &CameraWorker::start);

       thread1->start();
       thread2->start();
   }

Single-Camera vs Dual-Camera Build
-------------------------------------

The same codebase supports both configurations. Use a compile-time
constant or a config file to select the number of cameras:

.. code-block:: cpp

   // In MainWindow.cpp — adapt based on config
   const int NUM_CAMERAS = 2;   // set to 1 for single-camera build

   QWidget* MainWindow::buildMainArea()
   {
       QWidget*     w      = new QWidget;
       QVBoxLayout* layout = new QVBoxLayout(w);
       layout->setContentsMargins(0, 0, 0, 0);
       layout->setSpacing(0);

       m_camPanel1 = new CameraPanel("Camera 1", QColor("#378ADD"),
                                     "192.168.10.41", "SFP+ 0", this);
       layout->addWidget(m_camPanel1, 1);

       if (NUM_CAMERAS == 2)
       {
           m_camPanel2 = new CameraPanel("Camera 2", QColor("#1D9E75"),
                                         "192.168.10.42", "SFP+ 1", this);
           layout->addWidget(m_camPanel2, 1);
       }
       return w;
   }

.. tip::
   Camera IP addresses, the number of cameras, output directory, and
   other deployment parameters should be read from a config file
   (e.g. a ``camera_config.json`` in the application directory) rather
   than hardcoded. This allows the same binary to be deployed on different
   systems without recompilation.
