# Installation Guide

This directory contains comprehensive installation instructions for the Jeep Sensor Hub system, including hardware setup, software installation, and configuration.

## Overview

The Jeep Sensor Hub installation process involves setting up hardware components, installing software on the Raspberry Pi Zero and ESP32 modules, configuring the system, and testing all functionality. This guide provides step-by-step instructions for a complete installation.

## Prerequisites

### Hardware Requirements
- **Raspberry Pi Zero W** (or newer)
- **ESP32 Development Boards** (3x for displays)
- **3" OLED Displays** (SSD1306 or SH1106)
- **Sensors**: DHT22, MPU6050, GPS module, pressure sensors
- **Power Management**: Voltage regulators, fuses, connectors
- **Enclosures**: IP65 rated for main controller, IP54 for displays

### Software Requirements
- **Operating System**: Raspberry Pi OS Lite (32-bit)
- **Python**: Python 3.9+ (included with Raspberry Pi OS)
- **MicroPython**: Latest stable version for ESP32
- **Development Tools**: Git, pip, text editor

### Network Requirements
- **WiFi Network**: 2.4GHz WiFi network for ESP32 modules
- **Local Network**: Ethernet or WiFi for Raspberry Pi
- **Internet Access**: For software updates and GPS time sync

## Installation Steps

### 1. Hardware Assembly

#### Main Controller (Raspberry Pi Zero)
1. **Mount Raspberry Pi Zero** in the main enclosure
2. **Install cooling** (heat sink and fan if needed)
3. **Connect power supply** (12V to 5V converter)
4. **Mount sensors** in appropriate locations
5. **Route cables** through designated openings

#### Display Modules (ESP32)
1. **Mount ESP32** in display enclosures
2. **Connect OLED display** via I2C
3. **Install buttons** for navigation
4. **Mount in vehicle** (dashboard, trunk, remote location)

#### Power Distribution
1. **Install main fuse** (30A) near battery
2. **Mount power distribution board**
3. **Connect auxiliary battery** to distribution board
4. **Route power cables** to all modules
5. **Install power source switch** for fallback

### 2. Software Installation

#### Raspberry Pi Zero Setup
```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install required packages
sudo apt install -y python3-pip python3-venv git

# Clone repository
git clone https://github.com/your-org/jeep-sensor-hub.git
cd jeep-sensor-hub

# Create virtual environment
python3 -m venv venv
source venv/bin/activate

# Install Python dependencies
pip install -r sensor-hub/requirements.txt

# Set up systemd service
sudo cp sensor-hub/jeep-sensor-hub.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable jeep-sensor-hub
```

#### ESP32 MicroPython Setup
```bash
# Install esptool
pip install esptool

# Download MicroPython firmware
wget https://micropython.org/resources/firmware/esp32-20220117-v1.18.bin

# Flash MicroPython to ESP32
esptool.py --chip esp32 --port /dev/ttyUSB0 erase_flash
esptool.py --chip esp32 --port /dev/ttyUSB0 write_flash -z 0x1000 esp32-20220117-v1.18.bin

# Upload project files
ampy --port /dev/ttyUSB0 put esp32-displays/primary-display/
ampy --port /dev/ttyUSB0 put esp32-displays/shared/
```

### 3. Configuration

#### Sensor Hub Configuration
```yaml
# sensor-hub/config.yaml
sensors:
  temperature:
    enabled: true
    gpio_pin: 4
    sensor_type: "DHT22"

  pressure:
    enabled: true
    adc_channel: 0
    sensor_type: "MPX5050"

  gps:
    enabled: true
    uart_tx: 14
    uart_rx: 15

power_management:
  enabled: true
  power_source_switch_pin: 23
  engine_signal_pin: 24
  system_led_pin: 25

cooling:
  enabled: true
  low_speed_threshold: 85
  high_speed_threshold: 95
  critical_threshold: 105
```

#### ESP32 Display Configuration
```python
# esp32-displays/config.py
WIFI_SSID = "YourWiFiNetwork"
WIFI_PASSWORD = "YourWiFiPassword"

SENSOR_HUB_IP = "192.168.1.100"
SENSOR_HUB_PORT = 8000

DISPLAY_CONFIG = {
    "type": "primary",  # primary, secondary, remote
    "orientation": "landscape",
    "brightness": 128,
    "timeout": 30000
}
```

#### Network Configuration
```bash
# Configure static IP (optional)
sudo nano /etc/dhcpcd.conf

# Add to end of file:
interface wlan0
static ip_address=192.168.1.100/24
static routers=192.168.1.1
static domain_name_servers=8.8.8.8

# Restart networking
sudo systemctl restart dhcpcd
```

### 4. System Testing

#### Hardware Testing
```bash
# Test sensors
cd sensor-hub
python test_components.py

# Test displays
cd esp32-displays
python test_displays.py

# Test power management
python -c "from src.power_management.power_manager import PowerManager; pm = PowerManager(); print('Power management OK')"
```

#### Network Testing
```bash
# Test WiFi connectivity
iwconfig
ping -c 4 8.8.8.8

# Test sensor hub API
curl http://localhost:8000/api/health

# Test WebSocket connection
python -c "import websocket; ws = websocket.create_connection('ws://localhost:8000/ws'); print('WebSocket OK')"
```

#### Integration Testing
```bash
# Start all services
sudo systemctl start jeep-sensor-hub

# Check service status
sudo systemctl status jeep-sensor-hub

# View logs
sudo journalctl -u jeep-sensor-hub -f

# Test web interface
curl http://localhost:8000/
```

### 5. Vehicle Integration

#### Mounting Locations
- **Main Controller**: Under dashboard or in trunk
- **Primary Display**: Dashboard, easily visible to driver
- **Secondary Display**: Trunk area for cargo monitoring
- **Remote Display**: Portable, battery-powered unit

#### Cable Routing
- **Power Cables**: Route along existing wire harnesses
- **Signal Cables**: Separate from high-power cables
- **Antennas**: Mount GPS antenna on roof or dashboard
- **Sensors**: Mount in appropriate engine compartments

#### Environmental Protection
- **Waterproofing**: Use IP65+ enclosures for main controller
- **Vibration**: Secure mounting with vibration dampeners
- **Temperature**: Ensure adequate ventilation and cooling
- **EMI Protection**: Route cables away from ignition systems

## Configuration Options

### Sensor Configuration
```yaml
# Temperature sensors
temperature:
  dht22:
    gpio_pin: 4
    update_interval: 5000
    calibration_offset: 0.0

  ds18b20:
    gpio_pin: 17
    update_interval: 2000
    calibration_offset: 0.0

# Pressure sensors
pressure:
  oil_pressure:
    adc_channel: 0
    voltage_divider: 2.0
    calibration_offset: 0.0

  fuel_pressure:
    adc_channel: 1
    voltage_divider: 3.0
    calibration_offset: 0.0
```

### Display Configuration
```python
# Display layout configuration
LAYOUTS = {
    "dashboard": {
        "primary": ["engine_temp", "oil_pressure", "voltage"],
        "secondary": ["gps_speed", "altitude", "heading"],
        "tertiary": ["coolant_temp", "fan_speed", "engine_load"]
    },
    "trunk": {
        "primary": ["cargo_temp", "humidity", "voltage"],
        "secondary": ["gps_position", "route_info"]
    },
    "remote": {
        "primary": ["system_status", "alerts"],
        "secondary": ["quick_controls"]
    }
}
```

### Power Management
```yaml
power_management:
  battery_monitoring:
    enabled: true
    low_voltage_threshold: 11.5
    critical_voltage_threshold: 10.5

  power_saving:
    enabled: true
    sleep_mode: "deep_sleep"
    wakeup_triggers: ["motion", "timer", "gpio"]

  charging:
    enabled: true
    max_charging_current: 2.0
    trickle_charging: true
```

## Troubleshooting

### Common Issues

#### Sensor Not Reading
```bash
# Check GPIO permissions
sudo usermod -a -G gpio $USER

# Test sensor directly
python -c "import RPi.GPIO as GPIO; GPIO.setmode(GPIO.BCM); print('GPIO OK')"

# Check wiring connections
sudo i2cdetect -y 1
```

#### Display Not Working
```bash
# Check I2C bus
sudo i2cdetect -y 1

# Test display connection
python -c "import smbus; bus = smbus.SMBus(1); print('I2C OK')"

# Verify power supply
multimeter 3.3V and 5V rails
```

#### Network Issues
```bash
# Check WiFi configuration
sudo iwconfig
sudo cat /etc/wpa_supplicant/wpa_supplicant.conf

# Test network connectivity
ping -c 4 192.168.1.1
nslookup google.com

# Check firewall
sudo ufw status
```

#### Power Problems
```bash
# Check power supply voltage
multimeter 12V input and 5V/3.3V outputs

# Check fuse continuity
multimeter fuse resistance

# Verify ground connections
multimeter ground continuity
```

### Debug Mode
```bash
# Enable debug logging
sudo nano /etc/systemd/system/jeep-sensor-hub.service

# Add to ExecStart:
--log-level DEBUG

# Restart service
sudo systemctl daemon-reload
sudo systemctl restart jeep-sensor-hub

# View debug logs
sudo journalctl -u jeep-sensor-hub -f
```

### Recovery Mode
```bash
# Boot into recovery mode
# Hold SHIFT during boot

# Mount SD card on another computer
# Edit configuration files
# Reboot normally
```

## Maintenance

### Regular Maintenance
- **Monthly**: Check all connections and mounting
- **Quarterly**: Clean sensors and displays
- **Annually**: Replace batteries and check power supplies
- **As Needed**: Update software and firmware

### Software Updates
```bash
# Update system packages
sudo apt update && sudo apt upgrade

# Update project code
cd jeep-sensor-hub
git pull origin main

# Update Python dependencies
pip install -r requirements.txt --upgrade

# Restart services
sudo systemctl restart jeep-sensor-hub
```

### Firmware Updates
```bash
# Update ESP32 firmware
esptool.py --chip esp32 --port /dev/ttyUSB0 write_flash -z 0x1000 new-firmware.bin

# Update MicroPython code
ampy --port /dev/ttyUSB0 put updated-code/
```

## Performance Optimization

### System Tuning
```bash
# Enable hardware acceleration
sudo raspi-config

# Optimize SD card performance
sudo nano /boot/config.txt
# Add: dtparam=sd_overclock=100

# Optimize memory usage
sudo nano /boot/cmdline.txt
# Add: cgroup_enable=cpuset cgroup_memory=1
```

### Sensor Optimization
```yaml
# Optimize update intervals
sensors:
  temperature:
    update_interval: 5000  # 5 seconds

  pressure:
    update_interval: 1000  # 1 second

  gps:
    update_interval: 1000  # 1 second
```

### Power Optimization
```yaml
# Power saving modes
power_management:
  sleep_mode: "light_sleep"
  wakeup_interval: 30000  # 30 seconds

  sensor_power_management:
    enabled: true
    sleep_when_idle: true
    wakeup_on_motion: true
```

## Security Considerations

### Network Security
```bash
# Change default passwords
sudo passwd pi
sudo passwd root

# Configure firewall
sudo ufw enable
sudo ufw allow ssh
sudo ufw allow 8000  # Sensor hub API
sudo ufw allow 3000  # Backend API

# Secure WiFi
sudo nano /etc/wpa_supplicant/wpa_supplicant.conf
# Use WPA2-PSK with strong password
```

### Access Control
```yaml
# API security
api:
  authentication:
    enabled: true
    jwt_secret: "your-secure-secret-key"
    token_expiry: "24h"

  rate_limiting:
    enabled: true
    max_requests: 100
    window_ms: 900000
```

## Support and Resources

### Documentation
- **Hardware Manuals**: Component datasheets and schematics
- **API Reference**: Complete API documentation
- **Troubleshooting Guide**: Common issues and solutions
- **Video Tutorials**: Installation and setup videos

### Community Support
- **GitHub Issues**: Bug reports and feature requests
- **Discord Server**: Community discussions and help
- **Email Support**: Direct support for enterprise users
- **User Forums**: Community knowledge sharing

### Professional Services
- **Installation Services**: Professional installation and setup
- **Customization**: Tailored configurations for specific vehicles
- **Training**: User and technician training programs
- **Maintenance Contracts**: Ongoing support and maintenance

## License

This installation guide is licensed under the MIT License. See the LICENSE file for details.
