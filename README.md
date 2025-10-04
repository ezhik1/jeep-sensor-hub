# Jeep Sensor Hub


## Backend

- Sensor HUB and ECU
	- Responsible for all reads and write on various sensors
- Web-API
	- Frontend for logging, and some configuration


## Hardware UI

- Provides the main human interface for setting ECU configurations, thresholds, and hardware IDs
- Runs on a Raspberry Pi, with a custom LVGL UI
- Live Data updates via local web API

### Setup

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

#### 1.2 Environment Variables

A `.env` file will be setup from the `.env.default` file when running `setup_dependencies.sh`. Be sure to confirm the values created before continuing the setup script, as there are specific values that need to correspond to your hardware:
	- Username
	- Project root directory
	- UI service characteristics
	- Display Rotation

#### 2. Build UI

```bash
cd pi-ui
mkdir build && cd build
cmake ..
make
```

#### 3. Reboot

```bash
sudo reboot
```

The device will auto-start the UI