#!/usr/bin/env bash
# ============================================================
#  05_gnss_sim.sh — GNSS NMEA simulator
#  Mode A (local):  bash 05_gnss_sim.sh local
#  Mode B (tcp):    bash 05_gnss_sim.sh tcp [port]
#  Mode C (nuc):    bash 05_gnss_sim.sh nuc
# ============================================================
set -euo pipefail
MODE="${1:-local}"; TCP_PORT="${2:-5555}"
NUC_HOST="192.168.1.50"; NUC_USER="${NUC_USER:-lvs}"
LAPTOP_IP=$(ip route get 8.8.8.8 2>/dev/null | grep -oP 'src \K\S+' || echo "192.168.1.x")

generate_nmea() {
    while true; do
        T=$(date -u +"%H%M%S.000"); D=$(date -u +"%d%m%y")
        printf "\$GNGGA,%s,4808.2292,N,01134.5674,E,1,09,0.9,524.3,M,47.8,M,,*40\r\n" "${T}"
        printf "\$GNRMC,%s,A,4808.2292,N,01134.5674,E,0.00,0.00,%s,,,A*6F\r\n" "${T}" "${D}"
        printf "\$GPGSA,A,3,01,02,03,04,05,06,07,08,09,,,,1.2,0.9,0.8*26\r\n"
        sleep 1
    done
}

case "${MODE}" in
  local)
    echo "GNSS sim → /tmp/ttyGNSS  (use --gnss-port /tmp/ttyGNSS)"
    pkill -f "socat.*ttyGNSS" 2>/dev/null || true; sleep 0.3
    socat PTY,link=/tmp/ttyGNSS,raw,echo=0 PTY,link=/tmp/ttyGNSS_w,raw,echo=0 &
    SPID=$!; sleep 0.5
    trap "kill ${SPID} 2>/dev/null; echo Stopped" EXIT
    generate_nmea > /tmp/ttyGNSS_w
    ;;
  tcp)
    echo "GNSS sim → TCP :${TCP_PORT}  (on NUC: socat /dev/ttyUSBx TCP:${LAPTOP_IP}:${TCP_PORT})"
    generate_nmea | socat - TCP-LISTEN:${TCP_PORT},reuseaddr,fork
    ;;
  nuc)
    echo "GNSS sim → NUC /tmp/ttyGNSS via SSH"
    ssh "${NUC_USER}@${NUC_HOST}" "pkill -f 'socat.*ttyGNSS' 2>/dev/null||true; sleep 0.3; \
        socat PTY,link=/tmp/ttyGNSS,raw,echo=0 PTY,link=/tmp/ttyGNSS_w,raw,echo=0 & \
        sleep 0.5; echo ready"
    trap "ssh ${NUC_USER}@${NUC_HOST} 'pkill -f socat 2>/dev/null||true'; echo Stopped" EXIT
    generate_nmea | ssh "${NUC_USER}@${NUC_HOST}" "cat > /tmp/ttyGNSS_w"
    ;;
  *)
    echo "Usage: $0 [local|tcp|nuc]"; exit 1 ;;
esac
