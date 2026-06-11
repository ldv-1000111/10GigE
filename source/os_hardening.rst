Production OS Hardening
========================

This chapter covers the steps needed to take the NUC13ANHi7 (and later
the Bedrock V3000) from a development-grade Ubuntu installation to a
production-grade deployment. The laptop is explicitly excluded — it
remains a standard development machine.

The goal is a system that:

- Boots directly into the acquisition daemon with no user session
- Survives power loss and hardware faults without manual intervention
- Exposes the minimum network attack surface
- Has predictable real-time performance under sustained load

The steps are ordered from most impactful to least — apply as far as
your operational requirements demand.

.. note::
   These steps assume Stage 1 is fully verified and the systemd service
   is running correctly before hardening begins. Do not harden a system
   that has not been verified to work correctly first.

----

Part A — NUC13ANHi7
---------------------

A.1 — Enable systemd Service (prerequisite)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Before hardening, confirm the service is running correctly:

.. code-block:: bash

   sudo systemctl enable shr-backend
   sudo systemctl start  shr-backend
   sudo systemctl status shr-backend

   # Watch logs
   journalctl -u shr-backend -f

The service must start cleanly before proceeding.

A.2 — Remove Unnecessary Packages
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Reduce the attack surface and memory footprint by removing packages
that are not needed in production:

.. code-block:: bash

   # Remove desktop environment if installed
   sudo apt-get remove --purge -y \
       ubuntu-desktop gnome-shell gdm3 \
       snapd cups avahi-daemon \
       bluetooth bluez

   sudo apt-get autoremove --purge -y
   sudo apt-get clean

   # Disable snap entirely
   sudo systemctl disable --now snapd

A.3 — Disable Unnecessary Services
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   # Services not needed on a headless acquisition system
   sudo systemctl disable --now \
       ModemManager \
       wpa_supplicant \
       apport \
       whoopsie \
       kerneloops

   # Verify services that should remain enabled
   sudo systemctl is-enabled \
       shr-backend \
       usb-gadget \
       systemd-networkd \
       ssh

A.4 — Configure Automatic Security Updates
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Security updates only — no automatic reboots or feature upgrades:

.. code-block:: bash

   sudo apt-get install -y unattended-upgrades

   sudo tee /etc/apt/apt.conf.d/50unattended-upgrades << 'EOF'
   Unattended-Upgrade::Allowed-Origins {
       "${distro_id}:${distro_codename}-security";
   };
   Unattended-Upgrade::Automatic-Reboot "false";
   Unattended-Upgrade::Remove-Unused-Dependencies "true";
   EOF

   sudo dpkg-reconfigure -plow unattended-upgrades

A.5 — Harden SSH
^^^^^^^^^^^^^^^^^

Edit ``/etc/ssh/sshd_config``:

.. code-block:: bash

   sudo nano /etc/ssh/sshd_config

Set the following:

.. code-block:: text

   PasswordAuthentication no
   PermitRootLogin no
   X11Forwarding no
   MaxAuthTries 3
   ClientAliveInterval 300
   ClientAliveCountMax 2

.. warning::
   Only disable ``PasswordAuthentication`` after confirming that
   SSH key authentication works (see :doc:`laptop_setup` Step 9).
   Locking yourself out of a headless NUC requires a display and
   keyboard to recover.

Restart SSH:

.. code-block:: bash

   sudo systemctl restart ssh

A.6 — Configure UFW Firewall
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Allow only the ports the system actually uses:

.. code-block:: bash

   sudo apt-get install -y ufw

   # Default: deny all incoming, allow all outgoing
   sudo ufw default deny incoming
   sudo ufw default allow outgoing

   # Allow SSH from the local network only
   sudo ufw allow from 192.168.1.0/24 to any port 22

   # Allow Backend TCP status stream
   sudo ufw allow from 192.168.1.0/24 to any port 9100

   # Allow UDP trigger
   sudo ufw allow from 192.168.1.0/24 to any port 9001/udp

   # Allow Android tablet USB NCM subnet
   sudo ufw allow from 192.168.100.0/24

   sudo ufw --force enable
   sudo ufw status verbose

A.7 — Configure Watchdog
^^^^^^^^^^^^^^^^^^^^^^^^^^

The NUC13ANHi7 supports hardware watchdog. Enable it so the system
reboots automatically if it freezes:

.. code-block:: bash

   sudo apt-get install -y watchdog

   sudo tee /etc/watchdog.conf << 'EOF'
   watchdog-device = /dev/watchdog
   watchdog-timeout = 60
   interval = 10
   realtime = yes
   priority = 1

   # Reboot if load average exceeds 24 (3× core count for NUC i7)
   max-load-1 = 24

   # Monitor the shr-backend process
   pidfile = /run/shr-backend.pid
   EOF

   sudo systemctl enable --now watchdog

Add ``WatchdogSec=30`` and ``NotifyAccess=all`` to the ``[Service]``
section of ``shr-backend.service`` so systemd also monitors the daemon:

.. code-block:: bash

   sudo nano /etc/systemd/system/shr-backend.service

Add under ``[Service]``:

.. code-block:: ini

   WatchdogSec=30
   NotifyAccess=all
   Restart=always
   RestartSec=5s

Then reload:

.. code-block:: bash

   sudo systemctl daemon-reload
   sudo systemctl restart shr-backend

A.8 — Real-time Kernel (optional)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For the lowest possible acquisition jitter, install the Ubuntu
real-time kernel:

.. code-block:: bash

   # Ubuntu Pro required for RT kernel (free for personal use)
   sudo pro attach <token>
   sudo pro enable realtime-kernel

   sudo reboot

   # Verify after reboot
   uname -r
   # Expected: ....-realtime

.. note::
   The real-time kernel is optional for the NUC. With the existing
   ``SCHED_FIFO`` scheduling and buffer tuning from :doc:`nuc_setup`,
   the standard kernel is sufficient for Camera Simulator testing.
   The RT kernel becomes relevant when running real SHR cameras at
   sustained 1.25 GB/s with tight timing requirements.

A.9 — Verify Boot-to-Acquisition Time
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

After all hardening steps, measure the time from power-on to the first
TCP status message being available:

.. code-block:: bash

   # On the laptop — poll until backend responds
   time (until nc -z 192.168.1.50 9100 2>/dev/null; do sleep 0.5; done)

Target: under 30 seconds from power-on to TCP :9100 available.

----

Part B — Bedrock V3000
-----------------------

The V3000 production hardening follows the same steps as the NUC
(A.1 through A.9) with the following differences.

B.1 — Network Interface Names
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The V3000 has dual 10GbE SFP+ ports. Identify them:

.. code-block:: bash

   ip link show
   # Typically: enp1s0f0 and enp1s0f1 for the two SFP+ ports

Update the netplan configuration accordingly — assign static IPs on
both interfaces if using dual cameras:

.. code-block:: yaml

   network:
     version: 2
     renderer: networkd
     ethernets:
       enp1s0f0:
         addresses: [192.168.10.1/24]
       enp1s0f1:
         addresses: [192.168.11.1/24]

B.2 — MTU 9000 (Jumbo Frames)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The SHR 10GigE cameras require jumbo frames for sustained throughput.
Set MTU 9000 on both camera-facing interfaces:

.. code-block:: yaml

   network:
     version: 2
     renderer: networkd
     ethernets:
       enp1s0f0:
         mtu: 9000
         addresses: [192.168.10.1/24]
       enp1s0f1:
         mtu: 9000
         addresses: [192.168.11.1/24]

Verify after applying:

.. code-block:: bash

   ip link show enp1s0f0 | grep mtu
   # Expected: mtu 9000

B.3 — CPU Core Isolation
^^^^^^^^^^^^^^^^^^^^^^^^^^

Isolate cores for real-time acquisition threads, preventing the OS
scheduler from preempting them:

.. code-block:: bash

   sudo nano /etc/default/grub

Change the ``GRUB_CMDLINE_LINUX_DEFAULT`` line to:

.. code-block:: text

   GRUB_CMDLINE_LINUX_DEFAULT="quiet splash isolcpus=0,1,2,3 nohz_full=0,1,2,3 rcu_nocbs=0,1,2,3"

Apply and reboot:

.. code-block:: bash

   sudo update-grub
   sudo reboot

Then pin the acquisition threads in the application to the isolated
cores using ``pthread_setaffinity_np()`` or via the systemd service:

.. code-block:: ini

   # In shr-backend.service [Service] section:
   CPUAffinity=0 1 2 3

B.4 — NVMe Queue Depth
^^^^^^^^^^^^^^^^^^^^^^^^

Increase the NVMe I/O scheduler depth for maximum write throughput:

.. code-block:: bash

   sudo tee /etc/udev/rules.d/60-nvme-scheduler.rules << 'EOF'
   ACTION=="add|change", KERNEL=="nvme[0-9]n[0-9]", \
       ATTR{queue/scheduler}="none", \
       ATTR{queue/nr_requests}="1024"
   EOF

   sudo udevadm trigger

B.5 — Disable ECC Scrubbing During Acquisition (optional)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

On the V3000 with ECC DDR5, background memory scrubbing can introduce
microsecond-scale jitter. Disable it during critical acquisition windows
via the ``edac`` module:

.. code-block:: bash

   # Disable EDAC scrubbing
   echo 0 | sudo tee /sys/devices/system/edac/mc/mc*/sdram_scrub_rate

   # Re-enable between acquisition sessions
   echo 3600 | sudo tee /sys/devices/system/edac/mc/mc*/sdram_scrub_rate

.. note::
   This is an advanced optimisation for production deployments where
   frame timing is critical. For most applications, leaving ECC
   scrubbing enabled is the correct choice.

----

Hardening Checklist
---------------------

.. list-table::
   :header-rows: 1
   :widths: 5 50 20 25

   * - #
     - Step
     - NUC
     - V3000
   * - A.1
     - systemd service enabled and verified
     - Required
     - Required
   * - A.2
     - Unnecessary packages removed
     - Recommended
     - Required
   * - A.3
     - Unnecessary services disabled
     - Recommended
     - Required
   * - A.4
     - Automatic security updates
     - Recommended
     - Recommended
   * - A.5
     - SSH hardened (key-only, no root)
     - Required
     - Required
   * - A.6
     - UFW firewall configured
     - Required
     - Required
   * - A.7
     - Hardware watchdog enabled
     - Recommended
     - Required
   * - A.8
     - Real-time kernel
     - Optional
     - Recommended
   * - A.9
     - Boot-to-acquisition time verified
     - Recommended
     - Required
   * - B.1
     - Dual NIC static IP
     - N/A
     - Required
   * - B.2
     - MTU 9000 jumbo frames
     - N/A
     - Required
   * - B.3
     - CPU core isolation
     - N/A
     - Recommended
   * - B.4
     - NVMe queue depth
     - N/A
     - Recommended
   * - B.5
     - ECC scrubbing control
     - N/A
     - Optional
