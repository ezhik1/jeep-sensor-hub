#!/usr/bin/env bash
if [ -z "$1" ]; then echo "Usage: $0 user@host"; exit 1; fi
TARGET="$1"
ARCHIVE="esp_ui_full_port.zip"
scp "$ARCHIVE" "$TARGET":/tmp/
ssh "$TARGET" bash -s <<'REMOTE'
set -e
sudo apt update
sudo apt install -y build-essential cmake libsdl2-dev libcurl4-openssl-dev python3-pip
cd /tmp
unzip -o esp_ui_full_port.zip -d /opt/esp_ui_full_port
cd /opt/esp_ui_full_port/pi-ui
mkdir -p build && cd build
cmake ..
make -j4 || true
echo "Built (or attempted build) pi-ui. Please inspect build output."
REMOTE
echo "Deployed (files copied)."
