GNSS UI — Android QML Components
===================================

The GNSS status panel lives entirely on the Android tablet as QML
components. The V3000 backend supplies GNSS data via the TCP status
message; the Android ``BackendClient`` exposes it as ``Q_PROPERTY``
values that QML binds to directly — no manual update calls needed.

GnssPanel.qml
--------------

``GnssPanel`` displays fix quality, live coordinates, satellite count,
HDOP, and UTC time. It reads directly from ``backend`` properties.

.. code-block:: qml

   // qml/GnssPanel.qml
   import QtQuick 2.15
   import QtQuick.Layouts 1.15
   import "styles/colors.js" as C

   Rectangle {
       color: "transparent"

       ColumnLayout {
           anchors { fill: parent; margins: 10 }
           spacing: 6

           // Section header + fix badge
           RowLayout {
               Layout.fillWidth: true

               Text {
                   text: "GNSS"
                   font { pixelSize: 10; weight: Font.Bold
                          letterSpacing: 0.9 }
                   color: C.txt2
               }
               Text {
                   text: "shared  ·  /dev/ttyUSB0"
                   font.pixelSize: 9; color: C.txt3
               }
               Item { Layout.fillWidth: true }

               // Fix quality badge
               Rectangle {
                   width: fixLabel.implicitWidth + 14
                   height: 18; radius: 3
                   color: backend.gnssValid
                          ? "#1a3a1a" : "#3a2a0a"
                   border.color: backend.gnssValid
                                 ? "#2a6a2a" : "#6a4a0a"

                   Row {
                       anchors.centerIn: parent
                       spacing: 4
                       Rectangle {
                           width: 5; height: 5; radius: 3
                           color: backend.gnssValid ? C.c2t : C.amber
                           anchors.verticalCenter: parent.verticalCenter
                       }
                       Text {
                           id: fixLabel
                           text: backend.gnssValid
                                 ? backend.gnssFix : "No fix"
                           font { pixelSize: 10; weight: Font.Bold
                                  letterSpacing: 0.4 }
                           color: backend.gnssValid ? C.c2t : C.amber
                       }
                   }
               }
           }

           // Satellite count + HDOP row
           Text {
               text: backend.gnssValid
                     ? backend.gnssSats + " sats  ·  HDOP "
                       + backend.gnssHdop.toFixed(1)
                     : "Waiting for fix..."
               font { pixelSize: 10; family: C.mono }
               color: C.txt2
           }

           // Coordinate grid — 2×2
           GridLayout {
               Layout.fillWidth: true
               columns: 2; rowSpacing: 5; columnSpacing: 5

               Repeater {
                   model: [
                       { lbl: "LAT",
                         val: backend.gnssLat.toFixed(6) + "°" },
                       { lbl: "LON",
                         val: backend.gnssLon.toFixed(6) + "°" },
                       { lbl: "ALT",
                         val: backend.gnssAlt.toFixed(1) + " m"  },
                       { lbl: "SPEED",
                         val: "0.0 kn"                           }
                   ]

                   Rectangle {
                       Layout.fillWidth: true
                       height: 38; radius: 5
                       color: C.bg2
                       border.color: C.bg3; border.width: 0.5

                       Column {
                           anchors { left: parent.left; leftMargin: 8
                                     verticalCenter: parent.verticalCenter }
                           spacing: 2
                           Text {
                               text: modelData.lbl
                               font { pixelSize: 9; letterSpacing: 0.6 }
                               color: C.txt2
                           }
                           Text {
                               text: backend.gnssValid
                                     ? modelData.val : "---"
                               font { pixelSize: 12; weight: Font.DemiBold
                                      family: C.mono }
                               color: C.txt0
                           }
                       }
                   }
               }
           }

           // UTC timestamp
           Text {
               text: backend.gnssValid ? backend.gnssUtc + " UTC" : ""
               font { pixelSize: 10; family: C.mono }
               color: C.txt2
               visible: backend.gnssValid
           }
       }

       // Bottom border
       Rectangle {
           anchors.bottom: parent.bottom
           width: parent.width; height: 1; color: "#2a2d31"
       }
   }

SyncPanel.qml
--------------

``SyncPanel`` shows the dual-camera synchronisation metrics — trigger
mode, frame delta, timestamp delta, total bandwidth, and dropped frames.

.. code-block:: qml

   // qml/SyncPanel.qml
   import QtQuick 2.15
   import QtQuick.Layouts 1.15
   import "styles/colors.js" as C

   Rectangle {
       color: "transparent"

       ColumnLayout {
           anchors { fill: parent; margins: 10 }
           spacing: 6

           // Header + live bandwidth
           RowLayout {
               Layout.fillWidth: true
               Text {
                   text: "SYNC"
                   font { pixelSize: 10; weight: Font.Bold
                          letterSpacing: 0.9 }
                   color: C.txt2
               }
               Item { Layout.fillWidth: true }
               Text {
                   text: backend.syncTotalBw.toFixed(2) + " GB/s"
                   font { pixelSize: 10; family: C.mono }
                   color: C.c1t
               }
           }

           // 2×2 metrics grid
           GridLayout {
               Layout.fillWidth: true
               columns: 2; rowSpacing: 5; columnSpacing: 5

               Repeater {
                   model: [
                       {
                           val: backend.syncDropped.toString(),
                           lbl: "dropped",
                           ok: backend.syncDropped === 0
                       },
                       {
                           val: backend.syncDeltaMs < 1
                                ? "<1ms"
                                : backend.syncDeltaMs.toFixed(1) + "ms",
                           lbl: "Δ timestamp",
                           ok: backend.syncDeltaMs < 5
                       },
                       {
                           val: "ActCmd",
                           lbl: "trigger",
                           ok: true
                       },
                       {
                           val: "1",
                           lbl: "Δ frames",
                           ok: true
                       }
                   ]

                   Rectangle {
                       Layout.fillWidth: true
                       height: 38; radius: 5
                       color: C.bg2
                       border.color: C.bg3; border.width: 0.5

                       Column {
                           anchors.centerIn: parent
                           spacing: 1

                           Text {
                               text: modelData.val
                               font { pixelSize: 13; weight: Font.Bold
                                      family: C.mono }
                               color: modelData.ok ? C.c2t : C.amber
                               anchors.horizontalCenter: parent.horizontalCenter
                           }
                           Text {
                               text: modelData.lbl
                               font.pixelSize: 9; color: C.txt2
                               anchors.horizontalCenter: parent.horizontalCenter
                           }
                       }
                   }
               }
           }
       }

       // Bottom border
       Rectangle {
           anchors.bottom: parent.bottom
           width: parent.width; height: 1; color: "#2a2d31"
       }
   }

How GNSS Data Flows
--------------------

.. code-block:: text

   /dev/ttyUSB0 (GNSS receiver RS232)
        │
        ▼
   NmeaReader (V3000 backend thread)
        │  atomically updates g_currentGnss
        ▼
   BackendServer::buildStatusPayload()
        │  reads g_currentGnss → JSON gnss{} object
        │  pushed to Android every 100ms (10 Hz)
        ▼
   TCP :9100 → USB NCM → Android tablet
        │
        ▼
   BackendClient::parseStatus()
        │  updates m_gnssLat, m_gnssLon, etc.
        │  emits statusUpdated()
        ▼
   QML property bindings
        │  backend.gnssLat, backend.gnssValid, etc.
        ▼
   GnssPanel.qml re-renders automatically

No slots, no manual ``setText()`` calls, no timers in the Android UI.
QML property bindings re-evaluate the moment ``statusUpdated()`` fires.
