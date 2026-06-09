#!/usr/bin/env bash
# ============================================================
#  01_setup_nuc.sh
#  Initial NUC setup — run ONCE from the laptop via SSH
#
#  Usage:
#    ssh luis@192.168.1.50 "bash -s" < scripts/01_setup_nuc.sh
#
#  What it does:
#    - Assigns static IP 192.168.1.50 (confirms it's already set)
#    - Creates deployment directories
#    - Installs Qt6 runtime libs (no SDK — runtime only)
#    - Installs USB serial support for GNSS
#    - Adds user to dialout group for /dev/ttyUSBx access
#    - Configures USB NCM gadget for Android tablet
#    - Sets OS tuning (MTU, buffers, real-time scheduling)
#    - Creates systemd service placeholder
# ============================================================
set -euo pipefail

NUC_USER="${USER}"
DEPLOY_DIR="/home/${NUC_USER}/shr"
VIMBA_LIB_DIR="/home/${NUC_USER}/vimba_libs"
FRAMES_DIR="/home/${NUC_USER}/frames"
NUC_IP="192.168.1.50"
ETH_IFACE=$(ip -o link show | awk -F': ' '{print $2}' | grep -E '^en' | head -1)

echo "============================================================"
echo " NUC13ANHi7 — Initial Setup"
echo " User:       ${NUC_USER}"
echo " Deploy dir: ${DEPLOY_DIR}"
echo " NUC IP:     ${NUC_IP}"
echo " ETH iface:  ${ETH_IFACE}"
echo "============================================================"

# ── Step 1: create directories ──────────────────────────────
echo ""
echo "[1/8] Creating deployment directories..."
mkdir -p "${DEPLOY_DIR}"
mkdir -p "${VIMBA_LIB_DIR}"
mkdir -p "${FRAMES_DIR}"
echo "  ✓ ${DEPLOY_DIR}"
echo "  ✓ ${VIMBA_LIB_DIR}"
echo "  ✓ ${FRAMES_DIR}"

# ── Step 2: system packages ─────────────────────────────────
echo ""
echo "[2/8] Installing runtime packages..."
sudo apt-get update -qq
sudo apt-get install -y \
    libqt6core6 libqt6network6 libqt6serialport6 \
    libusb-1.0-0 \
    socat \
    net-tools iproute2 \
    libkmod2

echo "  ✓ Runtime packages installed"

# ── Step 3: static IP (verify / apply) ──────────────────────
echo ""
echo "[3/8] Configuring static IP ${NUC_IP} on ${ETH_IFACE}..."

NETPLAN_FILE="/etc/netplan/01-shr-static.yaml"
sudo tee "${NETPLAN_FILE}" > /dev/null << NETPLAN
network:
  version: 2
  renderer: networkd
  ethernets:
    ${ETH_IFACE}:
      addresses:
        - ${NUC_IP}/24
      nameservers:
        addresses: [8.8.8.8, 8.8.4.4]
      routes:
        - to: default
          via: 192.168.1.1
NETPLAN

sudo chmod 600 "${NETPLAN_FILE}"
sudo netplan apply 2>/dev/null || true
echo "  ✓ Static IP ${NUC_IP} configured"

# ── Step 4: USB serial (GNSS) ───────────────────────────────
echo ""
echo "[4/8] Configuring USB serial for GNSS..."
sudo usermod -aG dialout "${NUC_USER}"
# Load cp210x and ftdi_sio for common USB-serial adapters
sudo modprobe cp210x  2>/dev/null || true
sudo modprobe ftdi_sio 2>/dev/null || true
echo "cp210x"  | sudo tee -a /etc/modules > /dev/null 2>&1 || true
echo "ftdi_sio" | sudo tee -a /etc/modules > /dev/null 2>&1 || true
echo "  ✓ dialout group, USB serial modules loaded"

# ── Step 5: USB NCM gadget for Android ──────────────────────
echo ""
echo "[5/8] Configuring USB NCM gadget (Android tablet link)..."
sudo modprobe libcomposite 2>/dev/null || true
sudo modprobe usb_f_ncm    2>/dev/null || true

sudo tee /usr/local/bin/usb-gadget-setup.sh > /dev/null << 'GADGET'
#!/bin/bash
# USB NCM gadget — NUC → Android tablet
modprobe libcomposite
modprobe usb_f_ncm

GADGET=/sys/kernel/config/usb_gadget/shr_cam
mkdir -p ${GADGET}

echo 0x1d6b > ${GADGET}/idVendor
echo 0x0104 > ${GADGET}/idProduct
mkdir -p ${GADGET}/strings/0x409
echo "SHR-NUC"         > ${GADGET}/strings/0x409/manufacturer
echo "SHR Camera Ctrl" > ${GADGET}/strings/0x409/product
echo "SHR0001"         > ${GADGET}/strings/0x409/serialnumber

mkdir -p ${GADGET}/configs/c.1/strings/0x409
echo "CDC NCM"         > ${GADGET}/configs/c.1/strings/0x409/configuration
echo 250               > ${GADGET}/configs/c.1/MaxPower

mkdir -p ${GADGET}/functions/ncm.usb0
echo "02:11:22:33:44:55" > ${GADGET}/functions/ncm.usb0/host_addr
echo "02:11:22:33:44:56" > ${GADGET}/functions/ncm.usb0/dev_addr
ln -sf ${GADGET}/functions/ncm.usb0 ${GADGET}/configs/c.1/

UDC=$(ls /sys/class/udc 2>/dev/null | head -1)
if [ -n "${UDC}" ]; then
    echo "${UDC}" > ${GADGET}/UDC
    sleep 1
    ip addr add 192.168.100.1/24 dev usb0 2>/dev/null || true
    ip link set usb0 up 2>/dev/null || true
    echo "USB NCM gadget active on usb0 (192.168.100.1)"
else
    echo "No UDC found — USB NCM not available on this port"
fi
GADGET

sudo chmod +x /usr/local/bin/usb-gadget-setup.sh

sudo tee /etc/systemd/system/usb-gadget.service > /dev/null << 'SVCEOF'
[Unit]
Description=USB NCM Gadget (Android tablet link)
After=network.target

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/usr/local/bin/usb-gadget-setup.sh

[Install]
WantedBy=multi-user.target
SVCEOF

sudo systemctl daemon-reload
sudo systemctl enable usb-gadget
echo "  ✓ USB NCM gadget service installed (will activate on next boot or: sudo systemctl start usb-gadget)"

# ── Step 6: OS tuning ────────────────────────────────────────
echo ""
echo "[6/8] Applying OS tuning for camera acquisition..."

sudo tee /etc/sysctl.d/99-shr-camera.conf > /dev/null << 'SYSCTL'
# SHR camera acquisition tuning
net.core.rmem_max=33554432
net.core.wmem_max=33554432
net.core.rmem_default=33554432
net.core.wmem_default=33554432
SYSCTL

sudo sysctl -p /etc/sysctl.d/99-shr-camera.conf > /dev/null
echo "  ✓ Receive buffers set to 32 MB"

# ── Step 7: real-time scheduling limits ─────────────────────
echo ""
echo "[7/8] Configuring real-time scheduling..."

sudo tee /etc/security/limits.d/shr-realtime.conf > /dev/null << LIMITS
${NUC_USER}  -  rtprio    99
${NUC_USER}  -  memlock   unlimited
${NUC_USER}  soft  nice   -10
${NUC_USER}  hard  nice   -20
LIMITS

grep -qF "pam_limits.so" /etc/pam.d/common-session || \
    echo "session required pam_limits.so" | sudo tee -a /etc/pam.d/common-session > /dev/null

echo "  ✓ Real-time scheduling configured (re-login to activate)"

# ── Step 8: systemd service placeholder ─────────────────────
echo ""
echo "[8/8] Installing shr-backend systemd service placeholder..."

sudo tee /etc/systemd/system/shr-backend.service > /dev/null << SVCEOF
[Unit]
Description=SHR Camera Acquisition Backend
After=network.target usb-gadget.service
Wants=usb-gadget.service

[Service]
Type=simple
User=${NUC_USER}
WorkingDirectory=${DEPLOY_DIR}
ExecStart=${DEPLOY_DIR}/SHR_Backend
Restart=on-failure
RestartSec=3s
Environment="LD_LIBRARY_PATH=${VIMBA_LIB_DIR}"
Environment="GENICAM_GENTL64_PATH=${VIMBA_LIB_DIR}"
LimitRTPRIO=99
LimitMEMLOCK=infinity
StandardOutput=journal
StandardError=journal
SyslogIdentifier=shr-backend

[Install]
WantedBy=multi-user.target
SVCEOF

sudo systemctl daemon-reload
echo "  ✓ shr-backend.service installed (not enabled yet — enable after first deploy)"

echo ""
echo "============================================================"
echo " ✓ NUC setup complete"
echo ""
echo " Summary:"
echo "   Static IP:    ${NUC_IP} on ${ETH_IFACE}"
echo "   Deploy dir:   ${DEPLOY_DIR}"
echo "   Vimba libs:   ${VIMBA_LIB_DIR}"
echo "   Frames dir:   ${FRAMES_DIR}"
echo "   USB NCM:      192.168.100.1 (Android tablet link)"
echo "   GNSS serial:  /dev/ttyUSB0"
echo ""
echo " Next steps:"
echo "   1. Re-login (or reboot) to activate group + limits"
echo "   2. Run deploy.sh from laptop after building"
echo "============================================================"
