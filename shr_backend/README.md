# SHR Camera Backend

Headless acquisition daemon for the Bedrock V3000 / Intel NUC13ANHi7.

See the full documentation for setup and build procedures:
https://shr-10gige-bedrock-docs.readthedocs.io

## Build

```bash
source ~/.bashrc   # ensure VIMBAX_DIR is set

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

## Run (with Camera Simulator)

```bash
cd build
LD_LIBRARY_PATH=. GENICAM_GENTL64_PATH=. \
  ./SHR_Backend --simulator --gnss-port /dev/ttyUSB0
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
```
