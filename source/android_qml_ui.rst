Android QML UI Components
==========================

The Android UI is built entirely in QML — Qt's declarative language for
touch-native interfaces. Every component from the ForeFlight-style mockup
maps to a QML file.

colours.js — Shared Palette
-----------------------------

Define the colour palette once in a JS file and import it in every
QML component:

.. code-block:: javascript

   // styles/colors.js
   .pragma library

   var bg0    = "#111214"
   var bg1    = "#1a1c1f"
   var bg2    = "#222528"
   var bg3    = "#2c2f33"
   var bg4    = "#363a3f"

   var c1     = "#1a8cff"    // Camera 1 blue
   var c1t    = "#6ab8ff"    // Camera 1 blue tint
   var c2     = "#00c87a"    // Camera 2 teal
   var c2t    = "#5dddaa"    // Camera 2 teal tint

   var amber  = "#f5a623"
   var ambrt  = "#fcd068"
   var red    = "#e84040"
   var redt   = "#f08080"

   var txt0   = "#f0f2f5"
   var txt1   = "#a8adb5"
   var txt2   = "#636870"
   var txt3   = "#3d4149"

   var mono   = "Courier New"

MainView.qml — Root Layout
----------------------------

.. code-block:: qml

   // qml/MainView.qml
   import QtQuick 2.15
   import QtQuick.Controls 2.15
   import QtQuick.Layouts 1.15
   import "styles/colors.js" as C

   ApplicationWindow {
       id: root
       visible: true
       title: "SHR Camera Control"
       color: C.bg0

       // Lock to landscape
       Component.onCompleted: {
           contentOrientation = Qt.LandscapeOrientation
       }

       // Title bar
       header: Rectangle {
           height: 38
           color: C.bg1
           border.color: C.bg3
           border.width: 0

           RowLayout {
               anchors { fill: parent; leftMargin: 12; rightMargin: 12 }
               spacing: 10

               Text {
                   text: "SHR Camera Control"
                   font { pixelSize: 13; weight: Font.DemiBold }
                   color: C.txt0
               }
               Text {
                   text: "  |  Bedrock V3000  ·  Vimba X 2026-1"
                   font.pixelSize: 11; color: C.txt2
               }

               Item { Layout.fillWidth: true }

               // Connection badge
               Rectangle {
                   width: connLabel.implicitWidth + 16
                   height: 20; radius: 3
                   color: backend.connected
                          ? "#1a3a1a" : "#3a1a1a"
                   border.color: backend.connected
                                 ? "#2a6a2a" : "#6a2a2a"
                   Text {
                       id: connLabel
                       anchors.centerIn: parent
                       text: backend.connected
                             ? "⬤  Connected" : "⬤  Disconnected"
                       font.pixelSize: 10; font.weight: Font.Bold
                       color: backend.connected ? C.c2t : C.redt
                   }
               }
           }

           // Bottom border
           Rectangle {
               anchors.bottom: parent.bottom
               width: parent.width; height: 1
               color: "#2a2d31"
           }
       }

       // Main body — three columns
       RowLayout {
           anchors.fill: parent
           spacing: 0

           // Icon rail
           SideBar { width: 52; Layout.fillHeight: true }

           // Stacked camera previews
           ColumnLayout {
               Layout.fillWidth: true
               Layout.fillHeight: true
               spacing: 0

               CameraPanel {
                   Layout.fillWidth: true
                   Layout.fillHeight: true
                   camIndex: 1
                   accentColor: C.c1
                   accentTint: C.c1t
                   camName: "Camera 1"
                   ipAddress: "192.168.10.41"
                   portLabel: "SFP+ 0"
               }

               // Divider
               Rectangle { width: parent.width; height: 1; color: "#2a2d31" }

               CameraPanel {
                   Layout.fillWidth: true
                   Layout.fillHeight: true
                   camIndex: 2
                   accentColor: C.c2
                   accentTint: C.c2t
                   camName: "Camera 2"
                   ipAddress: "192.168.10.42"
                   portLabel: "SFP+ 1"
               }

               // Trigger + start/stop bar
               TriggerBar { Layout.fillWidth: true }
           }

           // Right panel
           RightPanel {
               width: 200
               Layout.fillHeight: true
           }
       }
   }

CameraPanel.qml
----------------

.. code-block:: qml

   // qml/CameraPanel.qml
   import QtQuick 2.15
   import QtQuick.Layouts 1.15
   import "styles/colors.js" as C

   Item {
       id: root
       property int    camIndex: 1
       property color  accentColor: C.c1
       property color  accentTint: C.c1t
       property string camName: "Camera 1"
       property string ipAddress: ""
       property string portLabel: ""

       // Alias to backend properties based on camIndex
       readonly property int    frames:  camIndex === 1
                                         ? backend.cam1Frames  : backend.cam2Frames
       readonly property double fps:     camIndex === 1
                                         ? backend.cam1Fps     : backend.cam2Fps
       readonly property double written: camIndex === 1
                                         ? backend.cam1Written : backend.cam2Written
       readonly property double bw:      camIndex === 1
                                         ? backend.cam1Bw      : backend.cam2Bw
       readonly property int    dropped: camIndex === 1
                                         ? backend.cam1Dropped : backend.cam2Dropped
       readonly property int    bufFree: camIndex === 1
                                         ? backend.cam1BufFree : backend.cam2BufFree
       readonly property bool   geoTag:  camIndex === 1
                                         ? backend.cam1GeoTag  : backend.cam2GeoTag

       ColumnLayout { anchors.fill: parent; spacing: 0 }

       // Header bar
       Rectangle {
           id: header
           Layout.fillWidth: true
           height: 28
           color: camIndex === 1 ? "#131822" : "#101a14"

           RowLayout {
               anchors { fill: parent; leftMargin: 10; rightMargin: 10 }
               spacing: 7

               Rectangle {
                   width: 7; height: 7; radius: 4
                   color: root.accentColor
               }
               Text {
                   text: root.camName
                   font { pixelSize: 11; weight: Font.DemiBold }
                   color: C.txt0
               }
               Text {
                   text: root.ipAddress
                   font { pixelSize: 10; family: C.mono }
                   color: C.txt2
               }
               Item { Layout.fillWidth: true }
               Rectangle {
                   width: portText.implicitWidth + 12
                   height: 16; radius: 3
                   color: Qt.rgba(root.accentColor.r,
                                  root.accentColor.g,
                                  root.accentColor.b, 0.12)
                   Text {
                       id: portText
                       anchors.centerIn: parent
                       text: root.portLabel
                       font { pixelSize: 9; weight: Font.Bold
                              family: C.mono }
                       color: root.accentTint
                   }
               }
           }
           // Accent top line
           Rectangle {
               anchors.top: parent.top
               width: parent.width; height: 2
               color: root.accentColor; opacity: 0.6
           }
       }

       // Preview
       Rectangle {
           Layout.fillWidth: true
           Layout.fillHeight: true
           color: "#0c1018"

           // Placeholder — replace with live thumbnail Image
           Text {
               anchors.centerIn: parent
               text: root.camName
               font { pixelSize: 48; family: C.mono; weight: Font.Bold }
               color: Qt.rgba(root.accentColor.r,
                              root.accentColor.g,
                              root.accentColor.b, 0.06)
           }

           // Overlay TL
           Rectangle {
               anchors { top: parent.top; left: parent.left
                         topMargin: 7; leftMargin: 8 }
               width: overlayCol.implicitWidth + 18
               height: overlayCol.implicitHeight + 10
               color: "#b3000000"; radius: 5
               border.color: "#33ffffff"
               border.width: 0.5

               Column {
                   id: overlayCol
                   anchors.centerIn: parent
                   spacing: 2
                   Text {
                       text: "Frame #" + root.frames
                       font { pixelSize: 11; weight: Font.DemiBold }
                       color: root.accentTint
                   }
                   Text {
                       text: "14192 × 10640"
                       font.pixelSize: 10; color: C.txt2
                   }
                   Text {
                       text: root.geoTag ? "⬤  GEO-TAGGED" : "⚠  NO GNSS"
                       font { pixelSize: 9; weight: Font.Bold
                              letterSpacing: 0.3 }
                       color: root.geoTag ? C.c2t : C.amber
                   }
               }
           }
       }

       // Metrics bar
       Rectangle {
           Layout.fillWidth: true
           height: 28
           color: "#e6000000"
           border.color: "#2a2d31"; border.width: 0

           RowLayout {
               anchors { fill: parent; leftMargin: 10; rightMargin: 10 }
               spacing: 0

               Repeater {
                   model: [
                       { v: root.frames.toLocaleString(), l: "frames",
                         ok: root.frames > 0 },
                       { v: root.fps.toFixed(1),          l: "fps",     ok: true },
                       { v: root.written.toFixed(0),      l: "GB",      ok: true },
                       { v: root.bufFree + "/5",          l: "buffers", ok: true },
                       { v: root.bw.toFixed(2),           l: "GB/s",    ok: true },
                       { v: root.dropped.toString(),      l: "dropped",
                         ok: root.dropped === 0 }
                   ]

                   Item {
                       Layout.fillWidth: true
                       height: parent.height

                       Column {
                           anchors.centerIn: parent
                           spacing: 0
                           Text {
                               text: modelData.v
                               font { pixelSize: 11; weight: Font.DemiBold
                                      family: C.mono }
                               color: modelData.ok ? C.txt0 : C.amber
                               anchors.horizontalCenter: parent.horizontalCenter
                           }
                           Text {
                               text: modelData.l
                               font.pixelSize: 8; color: C.txt2
                               anchors.horizontalCenter: parent.horizontalCenter
                           }
                       }

                       // Separator
                       Rectangle {
                           anchors { right: parent.right; top: parent.top
                                     bottom: parent.bottom; topMargin: 6
                                     bottomMargin: 6 }
                           width: 0.5; color: "#2a2d31"
                           visible: index < 5
                       }
                   }
               }
           }
       }
   }

SectionWidget.qml
------------------

.. code-block:: qml

   // qml/SectionWidget.qml
   import QtQuick 2.15
   import "styles/colors.js" as C

   Column {
       id: root
       width: parent.width

       property string title: ""
       property bool   expanded: false
       property alias  content: bodyLoader.sourceComponent
       property alias  headerRight: rightSlot.data

       // Header
       Rectangle {
           width: parent.width; height: 30
           color: headerMouse.containsMouse ? "#08ffffff" : "transparent"

           Row {
               anchors { left: parent.left; leftMargin: 10
                         verticalCenter: parent.verticalCenter }
               spacing: 6
               Text {
                   text: root.title.toUpperCase()
                   font { pixelSize: 10; weight: Font.Bold
                          letterSpacing: 0.9 }
                   color: C.txt2
               }
               Item { id: rightSlot; width: childrenRect.width }
           }

           Text {
               anchors { right: parent.right; rightMargin: 10
                         verticalCenter: parent.verticalCenter }
               text: root.expanded ? "▼" : "▶"
               font.pixelSize: 10; color: C.txt3
               Behavior on opacity { NumberAnimation { duration: 120 } }
           }

           MouseArea {
               id: headerMouse
               anchors.fill: parent
               hoverEnabled: true
               onClicked: root.expanded = !root.expanded
           }

           Rectangle {
               anchors.bottom: parent.bottom
               width: parent.width; height: 1; color: "#2a2d31"
           }
       }

       // Animated body
       Item {
           width: parent.width
           height: root.expanded ? bodyLoader.implicitHeight : 0
           clip: true

           Behavior on height {
               NumberAnimation { duration: 160; easing.type: Easing.InOutQuad }
           }

           Loader { id: bodyLoader; width: parent.width; active: true }
       }
   }

TriggerBar.qml
---------------

.. code-block:: qml

   // qml/TriggerBar.qml
   import QtQuick 2.15
   import QtQuick.Layouts 1.15
   import "styles/colors.js" as C

   Rectangle {
       height: 80
       color: C.bg1
       border.color: "#2a2d31"; border.width: 0

       Rectangle {
           anchors.top: parent.top
           width: parent.width; height: 1; color: "#2a2d31"
       }

       ColumnLayout {
           anchors { fill: parent; margins: 10 }
           spacing: 6

           // Main trigger button
           Rectangle {
               Layout.fillWidth: true
               height: 34; radius: 6
               color: trigMouse.pressed ? "#0a56a8"
                    : trigMouse.containsMouse ? "#0d6ecc"
                    : "#1a8cff"

               Behavior on color { ColorAnimation { duration: 80 } }

               Text {
                   anchors.centerIn: parent
                   text: "⚡  TRIGGER BOTH CAMERAS"
                   font { pixelSize: 13; weight: Font.Bold
                          letterSpacing: 0.5 }
                   color: "white"
               }

               MouseArea {
                   id: trigMouse
                   anchors.fill: parent
                   hoverEnabled: true
                   onClicked: backend.triggerBoth()
               }
           }

           // Start / Cam1 / Cam2 / Stop row
           RowLayout {
               Layout.fillWidth: true
               spacing: 5

               Repeater {
                   model: [
                       { label: "▶  Start all", action: "start",
                         bg: "#1a3a10", border: "#3a6a20", fg: "#5aaa2a" },
                       { label: "Cam 1",       action: "cam1",
                         bg: C.bg2, border: C.bg4, fg: C.txt1 },
                       { label: "Cam 2",       action: "cam2",
                         bg: C.bg2, border: C.bg4, fg: C.txt1 },
                       { label: "■  Stop all", action: "stop",
                         bg: "#3a1010", border: "#6a2020", fg: "#cc5555" }
                   ]

                   Rectangle {
                       Layout.fillWidth: true
                       height: 22; radius: 5
                       color: btnMouse.pressed
                              ? Qt.darker(modelData.bg, 1.2)
                              : modelData.bg
                       border.color: modelData.border
                       border.width: 0.5

                       Text {
                           anchors.centerIn: parent
                           text: modelData.label
                           font { pixelSize: 11; weight: Font.DemiBold }
                           color: modelData.fg
                       }

                       MouseArea {
                           id: btnMouse
                           anchors.fill: parent
                           onClicked: {
                               if      (modelData.action === "start") backend.startAll()
                               else if (modelData.action === "stop")  backend.stopAll()
                               else if (modelData.action === "cam1")  backend.triggerCam1()
                               else if (modelData.action === "cam2")  backend.triggerCam2()
                           }
                       }
                   }
               }
           }
       }
   }

FrameLog.qml
-------------

.. code-block:: qml

   // qml/FrameLog.qml
   import QtQuick 2.15
   import "styles/colors.js" as C

   ListView {
       id: log
       clip: true
       model: ListModel { id: logModel }

       // Auto-scroll to latest entry
       onCountChanged: positionViewAtEnd()

       // Connect to backend frame events
       Connections {
           target: backend
           function onFrameEvent(cam, index, geoTagged, processMs, ts) {
               // Keep last 200 entries
               if (logModel.count >= 200) logModel.remove(0)
               logModel.append({
                   ts: ts.substring(6),  // HH:mm:ss.zzz → ss.zzz
                   cam: cam,
                   index: index,
                   geo: geoTagged,
                   ms: processMs
               })
           }
       }

       delegate: Rectangle {
           width: log.width; height: 18
           color: "transparent"

           Row {
               anchors { left: parent.left; leftMargin: 6
                         verticalCenter: parent.verticalCenter }
               spacing: 5

               Text {
                   text: model.ts
                   font { pixelSize: 10; family: C.mono }
                   color: C.txt2; width: 46
               }
               Text {
                   text: "C" + model.cam
                   font { pixelSize: 10; family: C.mono; weight: Font.Bold }
                   color: model.cam === 1 ? C.c1t : C.c2t
                   width: 18
               }
               Text {
                   text: "#" + model.index + " · "
                       + (model.geo ? "geo ✓" : "no fix")
                       + " · " + model.ms + "ms"
                   font { pixelSize: 10; family: C.mono }
                   color: model.geo ? C.txt1 : C.amber
               }
           }

           Rectangle {
               anchors.bottom: parent.bottom
               width: parent.width; height: 0.5; color: "#2a2d31"
           }
       }
   }
