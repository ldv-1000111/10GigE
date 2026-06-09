#!/usr/bin/env bash
# ============================================================
#  03_deploy.sh
#  Deploy SHR_Backend binary + Vimba X libs to the NUC
#
#  Usage:
#    bash scripts/03_deploy.sh [--restart]
#
#  Options:
#    --restart   Restart shr-backend service after deploy
#
#  Requirements:
#    - SSH key-based auth to NUC (or will prompt for password)
#    - NUC setup complete (scripts/01_setup_nuc.sh already run)
# ============================================================
set -euo pipefail

NUC_HOST="192.168.1.50"
NUC_USER="${NUC_USER:-lvs}"
NUC_TARGET="${NUC_USER}@${NUC_HOST}"
DEPLOY_DIR="/home/${NUC_USER}/shr"
VIMBA_LIB_DIR="/home/${NUC_USER}/vimba_libs"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$(dirname "${SCRIPT_DIR}")/build"

RESTART=false
[[ "${1:-}" == "--restart" ]] && RESTART=true

echo "============================================================"
echo " SHR Backend Deploy"
echo " Target:    ${NUC_TARGET}"
echo " Deploy:    ${DEPLOY_DIR}"
echo " Libs:      ${VIMBA_LIB_DIR}"
echo " Restart:   ${RESTART}"
echo "============================================================"

# ── Verify build exists ──────────────────────────────────────
if [ ! -f "${BUILD_DIR}/SHR_Backend" ]; then
    echo ""
    echo "✗ Build not found at ${BUILD_DIR}/SHR_Backend"
    echo "  Run scripts/02_build.sh first"
    exit 1
fi

# ── Test SSH connectivity ────────────────────────────────────
echo ""
echo "[1/4] Testing SSH connection to ${NUC_TARGET}..."
ssh -o ConnectTimeout=5 -o BatchMode=yes "${NUC_TARGET}" "echo '  ✓ SSH OK'" || {
    echo ""
    echo "  SSH connection failed. Options:"
    echo "  1. Setup SSH key:  ssh-copy-id ${NUC_TARGET}"
    echo "  2. Check NUC is reachable:  ping ${NUC_HOST}"
    echo "  3. Check NUC SSH is running:  ssh ${NUC_TARGET}"
    exit 1
}

# ── Deploy binary ────────────────────────────────────────────
echo ""
echo "[2/4] Deploying binary..."
rsync -avz --progress \
    "${BUILD_DIR}/SHR_Backend" \
    "${NUC_TARGET}:${DEPLOY_DIR}/"
echo "  ✓ SHR_Backend deployed"

# ── Deploy Vimba X shared libs ───────────────────────────────
echo ""
echo "[3/4] Deploying Vimba X shared libs..."
LIBS=(
    "libVmbCPP.so"
    "libVmbImageTransform.so"
    "VimbaCameraSimulatorTL.cti"
)

for LIB in "${LIBS[@]}"; do
    if [ -f "${BUILD_DIR}/${LIB}" ]; then
        rsync -avz "${BUILD_DIR}/${LIB}" "${NUC_TARGET}:${VIMBA_LIB_DIR}/"
        echo "  ✓ ${LIB}"
    else
        echo "  ⚠ ${LIB} not found in build dir (skipping)"
    fi
done

# Also copy VmbC.xml (TL config) and any .cti files from Vimba install
VIMBAX_DIR="${VIMBAX_DIR:-${HOME}/VimbaX_2026-1}"
if [ -f "${VIMBAX_DIR}/bin/VmbC.xml" ]; then
    rsync -avz "${VIMBAX_DIR}/bin/VmbC.xml" "${NUC_TARGET}:${VIMBA_LIB_DIR}/"
    echo "  ✓ VmbC.xml"
fi

# Copy all .cti files
find "${VIMBAX_DIR}/cti" -name "*.cti" 2>/dev/null | while read CTI; do
    rsync -avz "${CTI}" "${NUC_TARGET}:${VIMBA_LIB_DIR}/"
    echo "  ✓ $(basename ${CTI})"
done

# ── Restart service (optional) ───────────────────────────────
echo ""
echo "[4/4] Post-deploy..."
if $RESTART; then
    echo "  Restarting shr-backend service..."
    ssh "${NUC_TARGET}" "sudo systemctl restart shr-backend && \
        sleep 1 && \
        sudo systemctl status shr-backend --no-pager -l"
    echo "  ✓ Service restarted"
else
    echo "  Skipping service restart (use --restart flag to restart)"
    echo ""
    echo "  To start manually on the NUC:"
    echo "    ssh ${NUC_TARGET}"
    echo "    cd ${DEPLOY_DIR}"
    echo "    LD_LIBRARY_PATH=${VIMBA_LIB_DIR} GENICAM_GENTL64_PATH=${VIMBA_LIB_DIR} ./SHR_Backend"
fi

echo ""
echo "============================================================"
echo " ✓ Deploy complete"
echo ""
echo " Verify on NUC:"
echo "   ssh ${NUC_TARGET}"
echo "   journalctl -u shr-backend -f"
echo ""
echo " Test TCP stream from laptop:"
echo "   nc ${NUC_HOST} 9100"
echo ""
echo " Send a test trigger:"
echo "   echo '{\"type\":\"trigger\",\"target\":\"both\"}' | nc ${NUC_HOST} 9100"
echo "============================================================"
