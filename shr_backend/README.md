# SHR Camera Backend

Stage 1 — Headless acquisition daemon for the Bedrock V3000 / Intel NUC.

## Quick start

```bash
# 1. Install Vimba X on the laptop
bash scripts/00_install_vimba.sh
source ~/.bashrc

# 2. Setup the NUC (run once)
ssh lvs@192.168.1.50 "bash -s" < scripts/01_setup_nuc.sh

# 3. Build on the laptop
bash scripts/02_build.sh Release

# 4. Deploy to NUC
bash scripts/03_deploy.sh

# 5. Run on NUC manually (first time)
ssh lvs@192.168.1.50
cd ~/shr
LD_LIBRARY_PATH=~/vimba_libs GENICAM_GENTL64_PATH=~/vimba_libs \
  ./SHR_Backend --simulator --gnss-port /tmp/ttyGNSS

# 6. Simulate GNSS from laptop (separate terminal)
bash scripts/05_gnss_sim.sh nuc

# 7. Run Stage 1 tests
bash scripts/04_test_stage1.sh
```

## CLI options

| Option | Default | Description |
|---|---|---|
| `--simulator` / `-s` | off | Use Vimba X Camera Simulator |
| `--cam1 <id>` | auto | Camera 1 ID or IP |
| `--cam2 <id>` | auto | Camera 2 ID or IP |
| `--gnss-port <port>` | `/dev/ttyUSB0` | GNSS serial port |
| `--gnss-baud <baud>` | `9600` | GNSS baud rate |
| `--output <dir>` | `~/frames` | Frame output directory |
| `--port <port>` | `9100` | Backend TCP port |

## Test the TCP stream

```bash
# Watch status stream (10 Hz):
nc 192.168.1.50 9100

# Send a software trigger:
echo '{"type":"trigger","target":"both"}' | nc 192.168.1.50 9100

# Start acquisition:
echo '{"type":"start"}' | nc 192.168.1.50 9100
```

## Deploy as systemd service (after Stage 1 passes)

```bash
ssh lvs@192.168.1.50
sudo cp ~/shr/systemd/shr-backend.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now shr-backend
sudo systemctl status shr-backend
```
