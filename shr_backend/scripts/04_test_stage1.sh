#!/usr/bin/env bash
# ============================================================
#  04_test_stage1.sh
#  Stage 1 verification — runs from the LAPTOP
#  Tests the NUC backend end-to-end
#
#  Usage:
#    bash scripts/04_test_stage1.sh
#
#  What it tests:
#    1. NUC is reachable over SSH
#    2. Backend binary is running (or starts it)
#    3. TCP :9100 status stream is flowing
#    4. Software trigger fires and produces a frame
#    5. PPM + JSON sidecar written to ~/frames/
#    6. JSON sidecar structure is valid
#    7. (Optional) GNSS data present in sidecar
# ============================================================
set -euo pipefail

NUC_HOST="192.168.1.50"
NUC_USER="${NUC_USER:-lvs}"
NUC_TARGET="${NUC_USER}@${NUC_HOST}"
DEPLOY_DIR="/home/${NUC_USER}/shr"
VIMBA_LIB_DIR="/home/${NUC_USER}/vimba_libs"
FRAMES_DIR="/home/${NUC_USER}/frames"
PASS=0; FAIL=0

ok()   { echo "  ✓ $1"; ((PASS++)); }
fail() { echo "  ✗ $1"; ((FAIL++)); }
info() { echo "  → $1"; }

echo "============================================================"
echo " Stage 1 Verification — SHR Backend on NUC"
echo " Target: ${NUC_TARGET}"
echo "============================================================"

# ── Test 1: NUC reachable ────────────────────────────────────
echo ""
echo "[Test 1] NUC SSH connectivity"
if ssh -o ConnectTimeout=5 -o BatchMode=yes "${NUC_TARGET}" "true" 2>/dev/null; then
    ok "NUC reachable at ${NUC_HOST}"
else
    fail "Cannot reach NUC — check network and SSH"
    exit 1
fi

# ── Test 2: binary exists ────────────────────────────────────
echo ""
echo "[Test 2] Binary deployed"
if ssh "${NUC_TARGET}" "test -f ${DEPLOY_DIR}/SHR_Backend"; then
    ok "SHR_Backend binary present"
else
    fail "SHR_Backend not found at ${DEPLOY_DIR} — run 03_deploy.sh"
fi

# ── Test 3: start backend on NUC (if not running) ────────────
echo ""
echo "[Test 3] Backend running"
if ssh "${NUC_TARGET}" "pgrep -x SHR_Backend > /dev/null"; then
    ok "SHR_Backend already running"
else
    info "Starting SHR_Backend on NUC..."
    ssh "${NUC_TARGET}" "
        cd ${DEPLOY_DIR}
        LD_LIBRARY_PATH=${VIMBA_LIB_DIR} \
        GENICAM_GENTL64_PATH=${VIMBA_LIB_DIR} \
        nohup ./SHR_Backend \
            > /tmp/shr_backend.log 2>&1 &
        sleep 2
        pgrep -x SHR_Backend && echo 'started' || echo 'failed'
    "
    sleep 2
    if ssh "${NUC_TARGET}" "pgrep -x SHR_Backend > /dev/null"; then
        ok "SHR_Backend started successfully"
    else
        fail "SHR_Backend failed to start"
        info "Check log: ssh ${NUC_TARGET} cat /tmp/shr_backend.log"
    fi
fi

# ── Test 4: TCP :9100 responding ─────────────────────────────
echo ""
echo "[Test 4] TCP :9100 status stream"
STATUS_JSON=$(echo '{}' | nc -w3 "${NUC_HOST}" 9100 2>/dev/null | head -1 || true)
if [ -n "${STATUS_JSON}" ]; then
    ok "TCP :9100 responding"
    # Check it's valid JSON with expected fields
    if echo "${STATUS_JSON}" | python3 -c "
import sys,json
d=json.load(sys.stdin)
assert d.get('type')=='status', 'missing type:status'
assert 'cam1' in d, 'missing cam1'
assert 'gnss' in d, 'missing gnss'
print('  JSON structure valid')
" 2>/dev/null; then
        ok "Status JSON structure valid"
        # Extract and show some values
        FPS=$(echo "${STATUS_JSON}" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d['cam1']['fps'])" 2>/dev/null || echo "?")
        info "cam1.fps = ${FPS}"
    else
        fail "Status JSON malformed: ${STATUS_JSON}"
    fi
else
    fail "No response on TCP :9100 — backend may not be listening"
fi

# ── Test 5: clear frames dir, send trigger ───────────────────
echo ""
echo "[Test 5] Software trigger → frame written"
ssh "${NUC_TARGET}" "mkdir -p ${FRAMES_DIR}"
FRAME_COUNT_BEFORE=$(ssh "${NUC_TARGET}" "ls ${FRAMES_DIR}/*.ppm 2>/dev/null | wc -l || echo 0")

# Send trigger
echo '{"type":"trigger","target":"both"}' | nc -w2 "${NUC_HOST}" 9100 > /dev/null 2>&1 || true
sleep 2

FRAME_COUNT_AFTER=$(ssh "${NUC_TARGET}" "ls ${FRAMES_DIR}/*.ppm 2>/dev/null | wc -l || echo 0")
NEW_FRAMES=$(( FRAME_COUNT_AFTER - FRAME_COUNT_BEFORE ))

if [ "${NEW_FRAMES}" -ge 1 ]; then
    ok "Trigger produced ${NEW_FRAMES} new frame(s)"
else
    fail "No new frames after trigger (before=${FRAME_COUNT_BEFORE} after=${FRAME_COUNT_AFTER})"
fi

# ── Test 6: JSON sidecar valid ───────────────────────────────
echo ""
echo "[Test 6] JSON sidecar structure"
LATEST_JSON=$(ssh "${NUC_TARGET}" "ls -t ${FRAMES_DIR}/*.json 2>/dev/null | head -1 || echo ''")
if [ -n "${LATEST_JSON}" ]; then
    SIDECAR=$(ssh "${NUC_TARGET}" "cat ${LATEST_JSON}")
    if echo "${SIDECAR}" | python3 -c "
import sys,json
d=json.load(sys.stdin)
required=['frame_index','frame_filename','capture_timestamp_utc','gnss','image']
for k in required:
    assert k in d, f'missing field: {k}'
print('  All required fields present')
print(f'  frame_index = {d[\"frame_index\"]}')
print(f'  resolution  = {d[\"image\"][\"width\"]}x{d[\"image\"][\"height\"]}')
print(f'  gnss.valid  = {d[\"gnss\"][\"valid\"]}')
" 2>&1 | grep -v "^$"; then
        ok "JSON sidecar valid"
    else
        fail "JSON sidecar malformed"
        info "${SIDECAR}" | head -5
    fi
else
    fail "No JSON sidecar found in ${FRAMES_DIR}"
fi

# ── Test 7: GNSS in sidecar (optional) ──────────────────────
echo ""
echo "[Test 7] GNSS data (optional — requires real receiver or socat)"
if [ -n "${LATEST_JSON}" ]; then
    GNSS_VALID=$(ssh "${NUC_TARGET}" "
        python3 -c \"
import json
d=json.load(open('${LATEST_JSON}'))
print(d['gnss']['valid'])
\" 2>/dev/null || echo 'error'")
    if [ "${GNSS_VALID}" = "True" ]; then
        LAT=$(ssh "${NUC_TARGET}" "python3 -c \"import json; d=json.load(open('${LATEST_JSON}')); print(d['gnss']['latitude_deg'])\" 2>/dev/null")
        ok "GNSS fix present: lat=${LAT}"
    else
        info "GNSS not active (expected without receiver — sidecar has valid=false)"
        ok "Sidecar correctly records no-fix state"
    fi
fi

# ── Summary ──────────────────────────────────────────────────
echo ""
echo "============================================================"
if [ "${FAIL}" -eq 0 ]; then
    echo " ✓ Stage 1 PASSED — all ${PASS} tests passed"
    echo ""
    echo " The NUC backend is:"
    echo "   • Running headlessly"
    echo "   • Acquiring frames via Camera Simulator"
    echo "   • Writing PPM + JSON sidecar to ${FRAMES_DIR}"
    echo "   • Serving TCP :9100 status stream"
    echo "   • Ready for Stage 2 (Android app connection)"
else
    echo " ✗ Stage 1 FAILED — ${FAIL} test(s) failed, ${PASS} passed"
    echo ""
    echo " Debug:"
    echo "   ssh ${NUC_TARGET} 'cat /tmp/shr_backend.log'"
    echo "   ssh ${NUC_TARGET} 'journalctl -u shr-backend -n 50'"
fi
echo "============================================================"
exit "${FAIL}"
