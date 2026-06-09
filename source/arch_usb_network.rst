USB-C Network Link (USB NCM)
=============================

The V3000 and Android tablet communicate over a **USB NCM** (Network
Control Model) link — a standard USB protocol that makes the USB-C cable
appear as a wired Ethernet connection to both devices. No WiFi, no
Bluetooth, no external network infrastructure required.

How USB NCM Works
------------------

.. code-block:: text

   V3000 (USB gadget — NCM function)
        │
        │  USB-C cable
        │
   Android tablet (USB host — NCM driver, built into Android 13+)
        │
        └──► usb0 network interface → 192.168.100.2
                                V3000 gets 192.168.100.1

The V3000 presents itself as a USB peripheral (gadget) providing a
virtual network interface. Android sees it as a USB tethered network
connection and assigns an IP address automatically, or you assign
static IPs for determinism.

V3000 Setup — USB Gadget (NCM)
--------------------------------

**Step 1 — Load kernel modules:**

.. code-block:: bash

   sudo modprobe libcomposite
   sudo modprobe usb_f_ncm

Make permanent:

.. code-block:: bash

   echo "libcomposite" | sudo tee -a /etc/modules
   echo "usb_f_ncm"    | sudo tee -a /etc/modules

**Step 2 — Create a systemd service to configure the gadget on boot:**

.. code-block:: bash

   sudo nano /etc/systemd/system/usb-gadget.service

.. code-block:: ini

   [Unit]
   Description=USB NCM Gadget
   After=network.target

   [Service]
   Type=oneshot
   RemainAfterExit=yes
   ExecStart=/usr/local/bin/usb-gadget-setup.sh
   ExecStop=/usr/local/bin/usb-gadget-teardown.sh

   [Install]
   WantedBy=multi-user.target

**Step 3 — Create the setup script:**

.. code-block:: bash

   sudo nano /usr/local/bin/usb-gadget-setup.sh

.. code-block:: bash

   #!/bin/bash
   GADGET=/sys/kernel/config/usb_gadget/shr_cam

   modprobe libcomposite
   mkdir -p ${GADGET}

   echo 0x1d6b > ${GADGET}/idVendor     # Linux Foundation
   echo 0x0104 > ${GADGET}/idProduct    # Multifunction Composite
   echo 0x0100 > ${GADGET}/bcdDevice
   echo 0x0200 > ${GADGET}/bcdUSB

   mkdir -p ${GADGET}/strings/0x409
   echo "SolidRun"          > ${GADGET}/strings/0x409/manufacturer
   echo "SHR Camera Control"> ${GADGET}/strings/0x409/product
   echo "SHR0001"           > ${GADGET}/strings/0x409/serialnumber

   mkdir -p ${GADGET}/configs/c.1/strings/0x409
   echo "CDC NCM"           > ${GADGET}/configs/c.1/strings/0x409/configuration
   echo 250                 > ${GADGET}/configs/c.1/MaxPower

   mkdir -p ${GADGET}/functions/ncm.usb0
   # Optional: set fixed MAC addresses for determinism
   echo "02:11:22:33:44:55" > ${GADGET}/functions/ncm.usb0/host_addr
   echo "02:11:22:33:44:56" > ${GADGET}/functions/ncm.usb0/dev_addr

   ln -sf ${GADGET}/functions/ncm.usb0 ${GADGET}/configs/c.1/

   # Bind to the UDC (USB Device Controller)
   UDC=$(ls /sys/class/udc | head -1)
   echo "${UDC}" > ${GADGET}/UDC

   # Assign static IP to the USB network interface
   sleep 1
   ip addr add 192.168.100.1/24 dev usb0
   ip link set usb0 up

.. code-block:: bash

   sudo chmod +x /usr/local/bin/usb-gadget-setup.sh
   sudo systemctl enable usb-gadget
   sudo systemctl start usb-gadget

**Step 4 — Verify the interface is up:**

.. code-block:: bash

   ip addr show usb0
   # Expected:
   # usb0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500
   #     inet 192.168.100.1/24 scope global usb0

Android Setup
--------------

Android 13+ includes native USB NCM support. When the USB-C cable is
connected:

1. Android detects the V3000 as a USB network device
2. Go to **Settings → Network → USB** (or similar depending on OEM)
3. Select **USB tethering** or **USB networking** — the exact label
   varies by Android version and manufacturer
4. Android assigns itself ``192.168.100.2`` (or DHCP from the V3000)

To assign a static IP on Android (recommended for reliable connection):

.. code-block:: text

   Settings → Network & internet → Internet → [USB network]
   → IP settings: Static
   → IP address: 192.168.100.2
   → Gateway: 192.168.100.1
   → Subnet mask: 255.255.255.0

**Verify from Android:**

.. code-block:: bash

   # Using ADB shell (with tablet connected):
   adb shell ip addr show usb0
   adb shell ping 192.168.100.1 -c 3

Testing the Link
-----------------

From the V3000, ping the tablet:

.. code-block:: bash

   ping 192.168.100.2 -c 4
   # Expected round-trip time: < 1 ms

From a terminal on the V3000, test TCP connectivity:

.. code-block:: bash

   # Start a test listener on V3000
   nc -l 9100

   # From Android ADB shell — connect and send a test message
   adb shell "echo 'hello' | nc 192.168.100.1 9100"

Throughput
-----------

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - USB version
     - Theoretical max
     - Practical (TCP)
   * - USB 2.0
     - 480 Mbps
     - ~350–400 Mbps
   * - USB 3.0
     - 5 Gbps
     - ~2–3 Gbps
   * - USB 3.1 Gen2
     - 10 Gbps
     - ~4–6 Gbps

For the BackendServer protocol (JSON status at 10 Hz + command messages),
actual throughput is negligible — well under 1 Mbps. The link capacity is
vastly overprovisioned for this use case. The benefit is latency:
**round-trip time under 1 ms**, compared to 5–50 ms over WiFi.

.. note::
   If thumbnail images are added to the protocol in future (sending
   downsampled previews from V3000 to Android), USB 2.0 NCM can
   comfortably sustain ~10 JPEG thumbnails per second at 1920×1080
   quality without approaching its bandwidth limit.

ADB over USB NCM (Development)
--------------------------------

During development, Android Debug Bridge (ADB) can run over the USB NCM
network interface, allowing wireless-style ADB without needing a separate
USB connection:

.. code-block:: bash

   # On V3000 — after USB NCM link is established
   adb connect 192.168.100.2:5555

   # Deploy updated APK over the network link
   adb -s 192.168.100.2:5555 install SHR_Camera_Android.apk

   # View Android logs
   adb -s 192.168.100.2:5555 logcat | grep SHR
