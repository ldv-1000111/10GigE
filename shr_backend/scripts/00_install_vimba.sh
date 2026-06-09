#!/usr/bin/env bash
# ============================================================
#  00_install_vimba.sh
#  Install Vimba X 2026-1 SDK on the laptop (dev machine)
#  Ubuntu 24.04 x86_64
#
#  Run once:  bash scripts/00_install_vimba.sh
#
#  The script locates the Vimba X archive automatically by
#  searching common locations for any VimbaX*.tar.gz file.
#  Place the downloaded archive anywhere under ~/ before running.
# ============================================================
set -euo pipefail

VIMBA_VERSION="2026-1"
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

# ── Step 2: locate archive ───────────────────────────────────
echo ""
echo "[2/6] Locating Vimba X archive..."

ARCHIVE=""
for CANDIDATE in \
    "${HOME}"/VimbaX*.tar.gz \
    "${HOME}/Downloads"/VimbaX*.tar.gz \
    "${HOME}/Desktop"/VimbaX*.tar.gz
do
    # glob may not expand — check each
    for F in ${CANDIDATE}; do
        if [ -f "${F}" ]; then
            ARCHIVE="${F}"
            break 2
        fi
    done
done

if [ -z "${ARCHIVE}" ]; then
    echo ""
    echo "  ✗ No VimbaX*.tar.gz archive found."
    echo ""
    echo "  Please download Vimba X ${VIMBA_VERSION} for Linux x86_64 from:"
    echo "  https://www.alliedvision.com/en/support/software-downloads/"
    echo ""
    echo "  Place the downloaded .tar.gz anywhere under ~/ and re-run this script."
    exit 1
fi

echo "  ✓ Found archive: ${ARCHIVE}"
echo "  Verifying..."

# Verify it is actually a gzip archive
if ! file "${ARCHIVE}" | grep -q "gzip"; then
    echo ""
    echo "  ✗ ${ARCHIVE} is not a valid gzip archive."
    echo "  $(file "${ARCHIVE}")"
    echo ""
    echo "  The file may be a partial or failed download."
    echo "  Please re-download from alliedvision.com and try again."
    exit 1
fi
echo "  ✓ Archive is valid gzip"

# ── Step 3: extract ──────────────────────────────────────────
echo ""
echo "[3/6] Extracting..."

# Find the top-level directory name inside the archive
ARCHIVE_ROOT=$(tar -tzf "${ARCHIVE}" | head -1 | cut -d/ -f1)
echo "  Archive root: ${ARCHIVE_ROOT}"
echo "  Extracting to ${HOME}/${ARCHIVE_ROOT}..."

if [ -d "${HOME}/${ARCHIVE_ROOT}" ]; then
    echo "  Directory already exists, skipping extraction"
else
    tar -xzf "${ARCHIVE}" -C "${HOME}/"
    echo "  ✓ Extracted"
fi

# Create a stable symlink at the expected install dir
if [ "${HOME}/${ARCHIVE_ROOT}" != "${VIMBA_INSTALL_DIR}" ]; then
    echo "  Creating symlink: ${VIMBA_INSTALL_DIR} → ${HOME}/${ARCHIVE_ROOT}"
    ln -snf "${HOME}/${ARCHIVE_ROOT}" "${VIMBA_INSTALL_DIR}"
fi

echo "  ✓ Vimba X available at ${VIMBA_INSTALL_DIR}"

# ── Step 4: install GigE Transport Layer ─────────────────────
echo ""
echo "[4/6] Installing GigE Transport Layer..."

CTI_INSTALL=""
for CANDIDATE in \
    "${VIMBA_INSTALL_DIR}/cti/VimbaGigETL_Install.sh" \
    "${VIMBA_INSTALL_DIR}/Tools/Viewer/Bin/x86_64bit/VimbaGigETL_Install.sh"
do
    if [ -f "${CANDIDATE}" ]; then
        CTI_INSTALL="${CANDIDATE}"
        break
    fi
done

if [ -z "${CTI_INSTALL}" ]; then
    echo "  ⚠ VimbaGigETL_Install.sh not found — skipping"
    echo "  You may need to install the GigE TL manually."
    echo "  Look inside: ${VIMBA_INSTALL_DIR}"
else
    sudo bash "${CTI_INSTALL}"
    echo "  ✓ GigE TL installed"
fi

# ── Step 5: environment variables ────────────────────────────
echo ""
echo "[5/6] Setting up environment variables..."

PROFILE_FILE="${HOME}/.bashrc"

add_if_missing() {
    grep -qF "$1" "${PROFILE_FILE}" || echo "$1" >> "${PROFILE_FILE}"
}

add_if_missing "# Vimba X ${VIMBA_VERSION}"
add_if_missing "export VIMBAX_DIR=${VIMBA_INSTALL_DIR}"
add_if_missing "export LD_LIBRARY_PATH=\${VIMBAX_DIR}/api/lib/x86_64:\${LD_LIBRARY_PATH:-}"
add_if_missing "export GENICAM_GENTL64_PATH=\${VIMBAX_DIR}/cti"

export VIMBAX_DIR="${VIMBA_INSTALL_DIR}"
export LD_LIBRARY_PATH="${VIMBA_INSTALL_DIR}/api/lib/x86_64:${LD_LIBRARY_PATH:-}"
export GENICAM_GENTL64_PATH="${VIMBA_INSTALL_DIR}/cti"

echo "  ✓ Added to ${PROFILE_FILE}"

# ── Step 6: verify ───────────────────────────────────────────
echo ""
echo "[6/6] Verifying installation..."

# Find and report key paths
find "${VIMBA_INSTALL_DIR}" -name "VmbCPP.h"           | head -1 | \
    xargs -I{} echo "  ✓ VmbCPP.h: {}"
find "${VIMBA_INSTALL_DIR}" -name "libVmbCPP.so"        | head -1 | \
    xargs -I{} echo "  ✓ libVmbCPP.so: {}"
find "${VIMBA_INSTALL_DIR}" -name "libVmbImageTransform.so" | head -1 | \
    xargs -I{} echo "  ✓ libVmbImageTransform.so: {}"
find "${VIMBA_INSTALL_DIR}" -name "*Simulator*.cti"     | head -1 | \
    xargs -I{} echo "  ✓ Camera Simulator TL: {}" || \
    echo "  ⚠ Camera Simulator TL not found"
find "${VIMBA_INSTALL_DIR}" -name "VimbaX_Viewer"       | head -1 | \
    xargs -I{} echo "  ✓ Vimba X Viewer: {}" || \
    echo "  ⚠ Viewer not found"

qmake6 --version 2>/dev/null | head -1 | xargs -I{} echo "  ✓ Qt: {}" || \
    echo "  ⚠ Qt6 qmake not found"
cmake --version | head -1 | xargs -I{} echo "  ✓ {}"

# Update VIMBAX_DIR in CMakeLists.txt to use the actual path
HEADER=$(find "${VIMBA_INSTALL_DIR}" -name "VmbCPP.h" | head -1)
if [ -n "${HEADER}" ]; then
    ACTUAL_API=$(dirname "$(dirname "${HEADER}")")
    echo ""
    echo "  API path: ${ACTUAL_API}"
fi

echo ""
echo "============================================================"
echo " ✓ Vimba X ${VIMBA_VERSION} installed"
echo ""
echo " IMPORTANT: activate environment now:"
echo "   source ~/.bashrc"
echo ""
echo " Then verify:"
echo "   \${VIMBAX_DIR}/bin/VimbaX_Viewer"
echo "   (Camera Simulator should appear in the camera list)"
echo "============================================================"
