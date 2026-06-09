#!/usr/bin/env bash
# ============================================================
#  02_build.sh
#  Build SHR_Backend on the laptop dev machine
#
#  Usage:
#    bash scripts/02_build.sh [Release|Debug]
#
#  Output:
#    build/SHR_Backend
#    build/libVmbCPP.so
#    build/libVmbImageTransform.so
#    build/VimbaCameraSimulatorTL.cti  (for dev/testing)
# ============================================================
set -euo pipefail

BUILD_TYPE="${1:-Release}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "${SCRIPT_DIR}")"
BUILD_DIR="${PROJECT_DIR}/build"
VIMBAX_DIR="${VIMBAX_DIR:-${HOME}/VimbaX_2026-1}"

echo "============================================================"
echo " SHR Backend Build"
echo " Type:      ${BUILD_TYPE}"
echo " Source:    ${PROJECT_DIR}"
echo " Build dir: ${BUILD_DIR}"
echo " Vimba X:   ${VIMBAX_DIR}"
echo "============================================================"

# Verify Vimba X
if [ ! -f "${VIMBAX_DIR}/api/include/VmbCPP/VmbCPP.h" ]; then
    echo ""
    echo "✗ Vimba X not found at ${VIMBAX_DIR}"
    echo "  Run scripts/00_install_vimba.sh first, then source ~/.bashrc"
    exit 1
fi

# Configure
echo ""
echo "[1/3] Configuring..."
cmake -S "${PROJECT_DIR}" \
      -B "${BUILD_DIR}" \
      -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
      -DVIMBAX_DIR="${VIMBAX_DIR}" \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      2>&1 | tail -5

# Build
echo ""
echo "[2/3] Building..."
cmake --build "${BUILD_DIR}" -j"$(nproc)" 2>&1

# Verify
echo ""
echo "[3/3] Verifying output..."
[ -f "${BUILD_DIR}/SHR_Backend" ] && \
    echo "  ✓ SHR_Backend ($(du -sh "${BUILD_DIR}/SHR_Backend" | cut -f1))" || \
    { echo "  ✗ SHR_Backend not found"; exit 1; }

for LIB in libVmbCPP.so libVmbImageTransform.so; do
    [ -f "${BUILD_DIR}/${LIB}" ] && \
        echo "  ✓ ${LIB}" || \
        echo "  ✗ ${LIB} missing"
done

[ -f "${BUILD_DIR}/VimbaCameraSimulatorTL.cti" ] && \
    echo "  ✓ VimbaCameraSimulatorTL.cti" || \
    echo "  ⚠ Camera Simulator TL not copied (check CMakeLists.txt)"

echo ""
echo "============================================================"
echo " ✓ Build complete: ${BUILD_DIR}/SHR_Backend"
echo " Next: bash scripts/03_deploy.sh"
echo "============================================================"
