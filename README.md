# Jeep Sensor Hub

A comprehensive sensor monitoring and control system for engine management and various sensors, featuring real-time data visualization and configuration management.

## Architecture

### Backend
- **Sensor HUB and ECU**: Responsible for all sensor reads and writes
- **Web-API**: Exposes data, configuration methods, and provides logging for states over time

### Hardware UI
- **Raspberry Pi Interface**: Custom LVGL-based touchscreen UI






## Setup

### Initial Setup

#### 1. Run Setup

The setup script automatically creates a `.env` file from `.env.default` if it doesn't exist:

```bash
sudo ./setup_jeep_sensor_hub.sh
```

If you need to customize the configuration, edit the `.env` file before running the setup:

```bash
nano .env
sudo ./setup_jeep_sensor_hub.sh
```

#### 2. Environment Variables

A `.env` file will be setup from the `.env.default` file when running `setup_dependencies.sh`. Be sure to confirm the values created before continuing the setup script, as there are specific values that need to correspond to your hardware:
	- Username
	- Project root directory
	- UI service characteristics
	- Display Rotation

#### 3. Build UI

```bash
cd pi-ui
mkdir build && cd build
cmake ..
make
```

#### 4. Reboot

```bash
sudo reboot
```

The device will auto-start the UI

## Service Management

### Starting the Hub Service

```bash
# Start the service
sudo systemctl start jeep-sensor-hub

# Enable auto-start on boot
sudo systemctl enable jeep-sensor-hub

# Check service status
sudo systemctl status jeep-sensor-hub
```

### Stopping the Hub Service

```bash
# Stop the service
sudo systemctl stop jeep-sensor-hub

# Disable auto-start on boot
sudo systemctl disable jeep-sensor-hub

# Check service status
sudo systemctl status jeep-sensor-hub
```

### Restarting the Hub Service

```bash
# Restart the service
sudo systemctl restart jeep-sensor-hub

# Reload configuration and restart
sudo systemctl reload-or-restart jeep-sensor-hub
```

### Viewing Service Logs

```bash
# View recent logs
sudo journalctl -u jeep-sensor-hub

# Follow logs in real-time
sudo journalctl -u jeep-sensor-hub -f

# View logs from today
sudo journalctl -u jeep-sensor-hub --since today
```

## Development

### Building from Scratch

#### Clean Build (Recommended for Development)

```bash
# Navigate to project root
cd /path/to/jeep-sensor-hub

# Stop the service first
sudo systemctl stop jeep-sensor-hub

# Clean previous build
cd pi-ui
rm -rf build
mkdir build && cd build

# Configure and build
cmake ..
make -j$(nproc)

# Restart the service
sudo systemctl start jeep-sensor-hub
```

#### Full Clean and Rebuild

```bash
# Navigate to project root
cd /path/to/jeep-sensor-hub

# Stop the service
sudo systemctl stop jeep-sensor-hub

# Clean everything
cd pi-ui
rm -rf build
rm -rf CMakeCache.txt
rm -rf CMakeFiles
rm -rf cmake_install.cmake
rm -rf Makefile

# Recreate build directory
mkdir build && cd build

# Fresh configuration
cmake ..

# Build with all available cores
make -j$(nproc)

# Start the service
sudo systemctl start jeep-sensor-hub
```

#### Quick Rebuild (After Code Changes)

```bash
# Navigate to build directory
cd /path/to/jeep-sensor-hub/pi-ui/build

# Stop service
sudo systemctl stop jeep-sensor-hub

# Rebuild only changed files
make -j$(nproc)

# Start service
sudo systemctl start jeep-sensor-hub
```

### Debugging

#### Run UI in Debug Mode

```bash
# Stop the service
sudo systemctl stop jeep-sensor-hub-ui.service

# Run UI directly (for debugging)
cd /path/to/jeep-sensor-hub/pi-ui/build
sudo ./jeep_sensor_hub_ui

# Press Ctrl+C to stop
```

#### Check Build Configuration

```bash
cd /path/to/jeep-sensor-hub/pi-ui/build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

### Troubleshooting

#### Service Won't Start

```bash
# Check service status
sudo systemctl status jeep-sensor-hub

# Check logs for errors
sudo journalctl -u jeep-sensor-hub --since "10 minutes ago"

# Verify executable exists and has correct permissions
ls -la /path/to/jeep-sensor-hub/pi-ui/build/jeep_sensor_hub_ui
```

#### Build Failures

```bash
# Check for missing dependencies
sudo apt update
sudo apt install build-essential cmake liblvgl-dev

# Clean and rebuild
cd /path/to/jeep-sensor-hub/pi-ui
rm -rf build
mkdir build && cd build
cmake ..
make
```

#### Permission Issues

```bash
# Ensure correct ownership
sudo chown -R $USER:$USER /path/to/jeep-sensor-hub

# Ensure executable permissions
chmod +x /path/to/jeep-sensor-hub/pi-ui/build/jeep_sensor_hub_ui
```