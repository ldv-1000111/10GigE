Hardware Setup
==============

This section covers physical connections between the SHR 10GigE camera
and the **Bedrock V3000**. The V3000's native dual 10 GbE SFP+ ports
connect directly to the camera with no intermediate hardware.

Connection Topology
--------------------

The recommended direct connection is point-to-point — one SHR camera
per SFP+ port:

.. code-block:: text

   SHR 10GigE Camera
         │
         │  Cat 6+ Ethernet (or SFP+ DAC cable)
         │
   Bedrock V3000
   ├── SFP+ Port 0  ◄── SHR camera (dedicated 10 GbE lane)
   └── SFP+ Port 1  ◄── uplink / second camera / management

.. note::
   The V3000 uses SFP+ cages. You can connect via:

   - **SFP+ DAC (Direct Attach Copper)** cable — simplest, up to ~5 m
   - **SFP+ to RJ45 transceiver** + Cat 6A cable — up to 100 m
   - **SFP+ optical** transceiver + fibre — for very long runs

   For tested accessories see:
   https://www.alliedvision.com/en/products/accessories

Cabling Requirements
---------------------

- Use **Cat 6 or higher** rated Ethernet (if using RJ45 transceivers)
- Keep runs as short as practical — every meter of cable adds latency
  and potential for packet loss at 10 Gbps
- Avoid running camera cables alongside high-voltage or motor cables
- SFP+ DAC cables are preferred for rack or cabinet installations

One Camera Per Port
--------------------

Always connect one SHR camera to one dedicated SFP+ port. Do not use
an unmanaged switch between camera and host — GigE Vision discovery
and packet timing depend on point-to-point topology.

The V3000's second SFP+ port can be used for:

- A second SHR camera (dual-camera configurations)
- Uplink to a management network
- Connection to a results/storage server

Camera Power
-------------

SHR cameras require a dedicated power supply (not PoE). Ensure the power
supply meets the camera's voltage and current requirements before connecting.

.. warning::
   Never hot-plug or disconnect the SHR camera while Vimba X is running.
   Always call ``AcquisitionStop`` and close the camera handle before
   removing power or the cable.

NIC Configuration
------------------

After connecting, configure the SFP+ port on the V3000:

**Set MTU to 9000 (Jumbo Frames):**

.. code-block:: bash

   # Identify the SFP+ interface name
   ip link show | grep -E "^[0-9]+:"

   # Set MTU — replace enp1s0 with your actual interface name
   sudo ip link set enp1s0 down
   sudo ip link set enp1s0 mtu 9000
   sudo ip link set enp1s0 up

   # Verify
   ip link show enp1s0 | grep mtu

Expected output::

   2: enp1s0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 9000 ...

**Assign a static IP on the camera subnet:**

.. code-block:: bash

   # Example: camera subnet 192.168.10.0/24
   sudo ip addr add 192.168.10.1/24 dev enp1s0

The SHR camera can be configured with a static IP via Vimba X Viewer
before deployment, or left on DHCP if a DHCP server is on the subnet.
