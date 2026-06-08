Option 1 — GigE Vision Action Commands
=========================================

**Trigger over Ethernet (ToE)** is the recommended trigger method when the
external computer is connected to the same Ethernet network as the SHR camera.
The external computer sends a **UDP broadcast packet** — a GigE Vision Action
Command — which the camera receives directly and uses to fire the sensor.

No additional wiring is required. The trigger travels over the existing
10GigE network infrastructure.

.. code-block:: text

   External Computer                   Network                SHR Camera
   ─────────────────                  ─────────              ──────────
   ActionCommand UDP  ─────────────►  10GigE  ──────────►  Sensor fires
   broadcast packet                   switch or             │
   (any language,                     direct link           ▼
    any OS)                                           Frame transmitted
                                                      to Bedrock V3000

How Action Commands Work
--------------------------

A GigE Vision Action Command is a standard UDP packet addressed to the
broadcast address of the camera subnet. Every GigE Vision camera on that
subnet receives it. Cameras filter by three keys that must match between
the sender and the camera configuration:

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Key
     - Purpose
   * - ``ActionDeviceKey``
     - Identifies the authorised sender. Camera ignores packets where
       this key does not match its own configured value.
   * - ``ActionGroupKey``
     - Groups cameras together. Only cameras in the matching group fire.
   * - ``ActionGroupMask``
     - Sub-group bitmask. Allows selective triggering within a group.

All three keys must match for the camera to respond.

Part A — Configure the Camera (V3000 / Vimba X)
-------------------------------------------------

The V3000 Qt application configures the camera to accept Action Commands.
This is done once during camera setup, before ``AcquisitionStart``:

.. code-block:: cpp

   // --- Action Command camera configuration ---

   // 1. Set the device key — must match what the sender will use
   //    This is a 32-bit unsigned integer — choose any non-zero value
   setInt(camera, "ActionDeviceKey", 0xC0FFEE01);

   // 2. Select which Action slot to configure (Action1 is most common)
   setInt(camera, "ActionSelector", 1);

   // 3. Set the group key and mask for this action slot
   setInt(camera, "ActionGroupKey",  0x00000001);
   setInt(camera, "ActionGroupMask", 0x00000001);

   // 4. Set trigger source to Action1
   setEnum(camera, "TriggerSelector",   "FrameStart");
   setEnum(camera, "TriggerSource",     "Action1");
   setEnum(camera, "TriggerActivation", "RisingEdge");
   setEnum(camera, "TriggerMode",       "On");

   // 5. Start acquisition — camera now waits for Action Commands
   runCmd(camera, "AcquisitionStart");

.. warning::
   ``ActionDeviceKey`` must be set **every time the camera is opened**.
   It is not persisted across power cycles. Build it into your camera
   setup sequence alongside pixel format and resolution.

Part B — Send Action Commands (External Computer)
--------------------------------------------------

The external computer sends the trigger. This can be any PC, embedded
computer, or device capable of sending a UDP broadcast packet.

**Option B1 — Using Vimba X on the External Computer (C++)**

If the external computer also has Vimba X installed, it can use the
``VmbSystem`` API to send Action Commands without opening a camera:

.. code-block:: cpp

   // On the EXTERNAL computer — sends the trigger
   #include "VmbCPP/VmbCPP.h"
   using namespace VmbCPP;

   VmbSystem& system = VmbSystem::GetInstance();
   system.Startup();

   // These keys MUST match what was configured on the camera
   const VmbUint32_t deviceKey = 0xC0FFEE01;
   const VmbUint32_t groupKey  = 0x00000001;
   const VmbUint32_t groupMask = 0x00000001;

   // Send Action Command — broadcast to all cameras on the subnet
   // Interface index 0 = first NIC; iterate InterfacePtrVector for
   // multi-NIC hosts
   InterfacePtrVector interfaces;
   system.GetInterfaces(interfaces);

   for (const auto& iface : interfaces)
   {
       // Send Action1 command
       iface->SendActionCommand(
           deviceKey,    // ActionDeviceKey
           groupKey,     // ActionGroupKey
           groupMask,    // ActionGroupMask
           1             // Action number (matches ActionSelector = 1)
       );
   }

   system.Shutdown();

**Option B2 — Raw UDP (Any Language, No SDK Required)**

A GigE Vision Action Command is a well-specified UDP packet that any
language can construct. This is useful when the external computer cannot
install Vimba X (embedded Linux, Python, etc.):

.. code-block:: python

   # Python example — send GigE Vision Action Command
   # No SDK required — pure socket programming
   import socket
   import struct

   # GigE Vision Action Command packet structure
   # See GigE Vision Specification Section 16
   def make_action_command(device_key, group_key, group_mask, action_num=1):
       # GigE Vision command header
       status         = 0x0000   # no acknowledge requested
       msg_id         = 0x0100   # ACTION_CMD
       length         = 0x0010   # payload length = 16 bytes
       req_id         = 0x0001   # request ID (arbitrary)

       header = struct.pack('>HHHH',
           status, msg_id, length, req_id)
       payload = struct.pack('>IIII',
           device_key,   # ActionDeviceKey
           group_key,    # ActionGroupKey
           group_mask,   # ActionGroupMask
           action_num    # ActionCommandIndex
       )
       return header + payload

   # Broadcast to 255.255.255.255 on GigE Vision command port 3956
   sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
   sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
   sock.bind(('', 0))

   packet = make_action_command(
       device_key = 0xC0FFEE01,
       group_key  = 0x00000001,
       group_mask = 0x00000001
   )

   sock.sendto(packet, ('255.255.255.255', 3956))
   sock.close()
   print("Action Command sent")

.. note::
   To trigger only cameras on a specific subnet (e.g. ``192.168.10.255``
   for the ``192.168.10.0/24`` camera network), replace
   ``255.255.255.255`` with the subnet broadcast address. This prevents
   the command from propagating beyond the camera network.

**Option B3 — Qt on the External Computer (QUdpSocket)**

If the external computer runs a Qt application:

.. code-block:: cpp

   #include <QUdpSocket>
   #include <QByteArray>
   #include <QDataStream>
   #include <QHostAddress>

   void sendActionCommand(quint32 deviceKey,
                          quint32 groupKey,
                          quint32 groupMask,
                          quint8  actionNum = 1)
   {
       QByteArray packet;
       QDataStream ds(&packet, QIODevice::WriteOnly);
       ds.setByteOrder(QDataStream::BigEndian);

       // GigE Vision command header (8 bytes)
       ds << quint16(0x0000)   // status: no ack
          << quint16(0x0100)   // ACTION_CMD
          << quint16(0x0010)   // payload length = 16 bytes
          << quint16(0x0001);  // request ID

       // Payload (16 bytes)
       ds << deviceKey
          << groupKey
          << groupMask
          << quint32(actionNum);

       QUdpSocket socket;
       socket.writeDatagram(packet,
           QHostAddress::Broadcast,
           3956);               // GigE Vision command port
   }

   // Call from a QPushButton slot or timer:
   // sendActionCommand(0xC0FFEE01, 0x00000001, 0x00000001);

Scheduled Action Commands (Precise Future Time)
-------------------------------------------------

For applications requiring a trigger at a precise future moment — for
example, at a GPS-derived time — the GigE Vision standard supports
**Scheduled Action Commands**. The external computer specifies a future
PTP timestamp, and the camera fires at exactly that time.

This requires **PTP synchronisation** between the external computer,
the V3000, and the camera (all must share the same PTP grandmaster clock):

.. code-block:: cpp

   // Enable PTP on the camera (V3000 side, during camera setup):
   setEnum(camera, "PtpMode", "Slave");

   // Send a scheduled action command at a specific PTP timestamp
   // (nanoseconds since PTP epoch)
   VmbUint64_t fireAtNs = currentPtpTimeNs + 50000000ULL; // 50 ms from now

   iface->SendActionCommand(deviceKey, groupKey, groupMask, 1, fireAtNs);

.. tip::
   Combine Scheduled Action Commands with the GNSS UTC time from
   :doc:`gnss_architecture` for GPS-slaved frame timing — trigger the
   camera at a PTP timestamp derived from the GNSS fix time for
   precise aerial survey frame intervals.

Part C — Handle the Triggered Frame on the V3000
--------------------------------------------------

From the V3000's perspective, a frame triggered by an Action Command
arrives in ``FrameReceived()`` exactly like any other frame. No special
handling is needed. The GNSS tagging in :doc:`gnss_frame_tagging` works
identically regardless of how the trigger was issued.

The Qt application can optionally log which frames were triggered remotely
by monitoring the ``TriggerSoftware`` or action-related event features if
the camera firmware supports them.

Key Parameters Summary
-----------------------

.. list-table::
   :header-rows: 1
   :widths: 35 20 45

   * - Vimba X Feature
     - Set on
     - Value
   * - ``ActionDeviceKey``
     - Camera (V3000)
     - e.g. ``0xC0FFEE01`` — match on sender
   * - ``ActionSelector``
     - Camera (V3000)
     - ``1`` (Action1)
   * - ``ActionGroupKey``
     - Camera (V3000)
     - e.g. ``0x00000001``
   * - ``ActionGroupMask``
     - Camera (V3000)
     - e.g. ``0x00000001``
   * - ``TriggerSource``
     - Camera (V3000)
     - ``"Action1"``
   * - ``TriggerMode``
     - Camera (V3000)
     - ``"On"``
   * - UDP destination
     - Sender (ext. PC)
     - ``255.255.255.255:3956`` or subnet broadcast
   * - Packet ``ActionDeviceKey``
     - Sender (ext. PC)
     - Must equal camera ``ActionDeviceKey``

References
-----------

- Allied Vision Action Commands Application Note:
  https://www.alliedvision.com/assets/documents/products/cameras/various/Action-Commands_Appnote.pdf
- GigE Vision Standard (AIA):
  https://www.visiononline.org/vision-standards-details.cfm/vision-standards/GigE-Vision/id/17
- IEEE 1588 PTP:
  https://standards.ieee.org/ieee/1588/6825/
