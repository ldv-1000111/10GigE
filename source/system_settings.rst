System Settings — Bedrock V3000 on Linux
==========================================

The Linux kernel defaults are tuned for general workloads, not sustained
1.25 GB/s camera streams. These settings must be applied before running
the application. All commands assume the V3000 running Ubuntu 22.04 LTS
or equivalent.

OS Receive Buffer Size
-----------------------

The most critical single setting. Increase the socket receive buffer to
32 MB to prevent the NIC from dropping packets under sustained SHR load:

.. code-block:: bash

   sudo sysctl -w net.core.rmem_max=33554432
   sudo sysctl -w net.core.wmem_max=33554432
   sudo sysctl -w net.core.rmem_default=33554432
   sudo sysctl -w net.core.wmem_default=33554432

Make permanent (survives reboot):

.. code-block:: bash

   cat << 'EOF' | sudo tee /etc/sysctl.d/99-shr-camera.conf
   net.core.rmem_max=33554432
   net.core.wmem_max=33554432
   net.core.rmem_default=33554432
   net.core.wmem_default=33554432
   EOF

   sudo sysctl -p /etc/sysctl.d/99-shr-camera.conf

MTU — Jumbo Frames (9000)
--------------------------

Already covered in :doc:`hardware_setup`. To make permanent across reboots
using NetworkManager:

.. code-block:: bash

   # Find your connection name
   nmcli connection show

   # Set MTU permanently on the camera NIC connection
   nmcli connection modify "Wired connection 1" 802-3-ethernet.mtu 9000
   nmcli connection up "Wired connection 1"

GigE Burst Size in Vimba X
----------------------------

For the SHR 10GigE cameras, set ``GVSPBurstSize`` to 64 or higher in your
application (see :doc:`feature_configuration`). This is set in software,
not at the OS level, but it is part of the system tuning:

.. code-block:: cpp

   setInt(camera, "GVSPBurstSize", 64);

Real-Time Thread Scheduling
----------------------------

For deterministic frame delivery on the V3000's 8-core CPU, enable
real-time scheduling in Vimba X and configure user limits.

**Step 1 — Enable in Vimba X:**

Edit ``VmbC.xml`` in ``~/VimbaX_2026-1/bin/`` and set:

.. code-block:: xml

   <EnableLinuxRealTimeSchedulingPolicies>true</EnableLinuxRealTimeSchedulingPolicies>

**Step 2 — User limits** (replace ``username`` with your Linux user):

.. code-block:: bash

   sudo tee -a /etc/security/limits.conf << 'EOF'
   username  -  rtprio    99
   username  -  memlock   unlimited
   username  soft  nice     -10
   username  hard  nice     -20
   EOF

**Step 3 — PAM configuration:**

.. code-block:: bash

   echo "session required pam_limits.so" | sudo tee -a /etc/pam.d/common-session
   echo "session required pam_limits.so" | sudo tee -a /etc/pam.d/common-session-noninteractive

**Step 4 — Log out and back in**, then verify:

.. code-block:: bash

   # Run the app, then in another terminal:
   ps -eLo pid,tid,cls,rtprio,pri,cmd | grep SHR_Capture_App

Look for ``FF`` in the ``cls`` column (FIFO real-time). ``TS`` means
configuration did not take effect.

CPU Core Isolation (Optional — Maximum Throughput)
---------------------------------------------------

For the highest sustained throughput on the V3000, isolate cores 0–3
from the Linux scheduler so they are exclusively available to Vimba X
and the frame worker:

.. code-block:: bash

   # Add to GRUB_CMDLINE_LINUX_DEFAULT in /etc/default/grub:
   # isolcpus=0,1,2,3 nohz_full=0,1,2,3 rcu_nocbs=0,1,2,3

   sudo nano /etc/default/grub
   # Edit: GRUB_CMDLINE_LINUX_DEFAULT="quiet splash isolcpus=0,1,2,3"
   sudo update-grub
   sudo reboot

.. warning::
   CPU isolation removes those cores from general OS use. Only do this
   on a dedicated acquisition machine. Qt and the OS will run on cores 4–7.

NVMe Write Scheduling
----------------------

For sustained frame writes to NVMe, use the ``none`` or ``mq-deadline``
I/O scheduler (avoid ``cfq`` which adds latency):

.. code-block:: bash

   # Check current scheduler for your NVMe device (e.g. nvme0n1)
   cat /sys/block/nvme0n1/queue/scheduler

   # Set to none (best for NVMe)
   echo none | sudo tee /sys/block/nvme0n1/queue/scheduler

Make permanent via udev rule:

.. code-block:: bash

   echo 'ACTION=="add|change", KERNEL=="nvme*", ATTR{queue/scheduler}="none"' \
     | sudo tee /etc/udev/rules.d/60-nvme-scheduler.rules

Checklist
----------

Before running the application, verify all of the following:

.. list-table::
   :header-rows: 1
   :widths: 50 50

   * - Setting
     - Verification command
   * - MTU = 9000
     - ``ip link show enp1s0 | grep mtu``
   * - rmem_max = 33554432
     - ``sysctl net.core.rmem_max``
   * - Real-time scheduling enabled in VmbC.xml
     - Check file contents
   * - PAM limits configured
     - ``ulimit -r`` (should show 99)
   * - NVMe scheduler = none
     - ``cat /sys/block/nvme0n1/queue/scheduler``
