#!/usr/bin/env bash
# ============================================================
#  00_install_vimba.sh
#  Install Vimba X 2026-1 SDK on the laptop (dev machine)
#  Ubuntu 24.04 x86_64
#
#  Run once:  bash scripts/00_install_vimba.sh
# ============================================================
set -euo pipefail

VIMBA_VERSION="2026-1"
VIMBA_ARCHIVE="VimbaX_${VIMBA_VERSION}_Linux_x86_64.tar.gz"
VIMBA_URL="https://www.alliedvision.com/fileadmin/content/software/software/VimbaX/${VIMBA_ARCHIVE}"
VIMBA_INSTALL_DIR="${HOME}/VimbaX_${VIMBA_VERSION}"

echo "============================================================"
echo " Vimba X ${VIMBA_VERSION} — Developer Machine Install"
echo " Target: ${VIMBA_INSTALL_DIR}"
echo "============================================================"

# ── Step 1: system dependencies ─────────────────────────────
echo ""
echo "[1/6] Installing system dependencies..."
sudo apt-get update -qq
sudo apt-get install -y \
    build-essential cmake ninja-build \
    qt6-base-dev qt6-base-dev-tools \
    qt6-serialport-dev \
    libqt6serialport6-dev \
    wget curl \
    socat \
    python3 python3-pip \
    net-tools iproute2 \
    libusb-1.0-0 libusb-1.0-0-dev

echo "  ✓ System dependencies installed"

# ── Step 2: download Vimba X ────────────────────────────────
echo ""
echo "[2/6] Downloading Vimba X ${VIMBA_VERSION}..."
echo "  NOTE: Vimba X requires free registration at alliedvision.com"
echo "        Download manually if the direct URL fails:"
echo "        https://www.alliedvision.com/en/support/software-downloads/"
echo ""

if [ -f "${HOME}/${VIMBA_ARCHIVE}" ]; then
    echo "  Archive already present: ${HOME}/${VIMBA_ARCHIVE}"
elif [ -f "${HOME}/Downloads/${VIMBA_ARCHIVE}" ]; then
    echo "  Found in Downloads, copying..."
    cp "${HOME}/Downloads/${VIMBA_ARCHIVE}" "${HOME}/"
else
    echo "  Attempting download (may require login)..."
    wget -q --show-progress -O "${HOME}/${VIMBA_ARCHIVE}" "${VIMBA_URL}" || {
        echo ""
        echo "  ✗ Auto-download failed (login required)."
        echo "    Please download ${VIMBA_ARCHIVE} manually and place it in ${HOME}/"
        echo "    Then re-run this script."
        exit 1
    }
fi
echo "  ✓ Archive ready"

# ── Step 3: extract ─────────────────────────────────────────
echo ""
echo "[3/6] Extracting to ${VIMBA_INSTALL_DIR}..."
if [ -d "${VIMBA_INSTALL_DIR}" ]; then
    echo "  Directory exists, skipping extraction"
else
    tar -xzf "${HOME}/${VIMBA_ARCHIVE}" -C "${HOME}/"
    echo "  ✓ Extracted"
fi

# ── Step 4: install GigE Transport Layer ────────────────────
echo ""
echo "[4/6] Installing GigE Transport Layer..."
CTI_DIR="${VIMBA_INSTALL_DIR}/cti"
if [ -f "${CTI_DIR}/VimbaGigETL_Install.sh" ]; then
    sudo bash "${CTI_DIR}/VimbaGigETL_Install.sh"
    echo "  ✓ GigE TL installed"
else
    echo "  ✗ VimbaGigETL_Install.sh not found at ${CTI_DIR}"
    echo "    Check your Vimba X archive structure"
    exit 1
fi

# ── Step 5: environment variables ───────────────────────────
echo ""
echo "[5/6] Setting up environment variables..."
PROFILE_LINE_1="export VIMBAX_DIR=${VIMBA_INSTALL_DIR}"
PROFILE_LINE_2="export LD_LIBRARY_PATH=\${VIMBAX_DIR}/api/lib/x86_64:\${LD_LIBRARY_PATH:-}"
PROFILE_LINE_3="export GENICAM_GENTL64_PATH=\${VIMBAX_DIR}/cti"

PROFILE_FILE="${HOME}/.bashrc"

add_if_missing() {
    grep -qF "$1" "${PROFILE_FILE}" || echo "$1" >> "${PROFILE_FILE}"
}

add_if_missing "# Vimba X ${VIMBA_VERSION}"
add_if_missing "${PROFILE_LINE_1}"
add_if_missing "${PROFILE_LINE_2}"
add_if_missing "${PROFILE_LINE_3}"

# Also export for this session
export VIMBAX_DIR="${VIMBA_INSTALL_DIR}"
export LD_LIBRARY_PATH="${VIMBA_INSTALL_DIR}/api/lib/x86_64:${LD_LIBRARY_PATH:-}"
export GENICAM_GENTL64_PATH="${VIMBA_INSTALL_DIR}/cti"

echo "  ✓ Added to ${PROFILE_FILE}"

# ── Step 6: verify ──────────────────────────────────────────
echo ""
echo "[6/6] Verifying installation..."

# Check headers
HDR="${VIMBA_INSTALL_DIR}/api/include/VmbCPP/VmbCPP.h"
[ -f "${HDR}" ] && echo "  ✓ VmbCPP.h found" || { echo "  ✗ VmbCPP.h not found"; exit 1; }

# Check libs
LIB="${VIMBA_INSTALL_DIR}/api/lib/x86_64/libVmbCPP.so"
[ -f "${LIB}" ] && echo "  ✓ libVmbCPP.so found" || { echo "  ✗ libVmbCPP.so not found"; exit 1; }

# Check Camera Simulator TL
SIM="${VIMBA_INSTALL_DIR}/cti/VimbaCameraSimulatorTL.cti"
[ -f "${SIM}" ] && echo "  ✓ Camera Simulator TL found" || echo "  ⚠ Camera Simulator TL not found (may be named differently)"

# Check Qt
qmake6 --version && echo "  ✓ Qt6 available" || echo "  ⚠ Qt6 qmake not found"

# Check CMake
cmake --version | head -1 && echo "  ✓ CMake available"

echo ""
echo "============================================================"
echo " ✓ Vimba X ${VIMBA_VERSION} installed successfully"
echo ""
echo "  IMPORTANT: Run the following to activate environment:"
echo "    source ~/.bashrc"
echo ""
echo "  Then verify the Camera Simulator:"
echo "    ${VIMBA_INSTALL_DIR}/bin/VimbaX_Viewer"
echo "============================================================"
