# Raspberry Pi Zero Sensor Hub

This is the main sensor hub implementation for the Jeep Sensor Hub Research project. It runs on a Raspberry Pi Zero and interfaces with various vehicle sensors and systems.

## Features

### 🚗 **Engine Cooling Management**
- **Dual-speed fan control** with configurable temperature thresholds
- **Smart buffering logic** to prevent temperature spikes
- **A/C integration** for external fan requests
- **Manual override controls** for testing and emergency situations
- **Real-time temperature monitoring** with DS18B20 or DHT22 sensors
- **Automatic fan control** based on engine load and ambient conditions

### 🚙 **TPMS (Tire Pressure Monitoring)**
- **CC1101 radio module** for 433MHz sensor communication
- **Multi-sensor support** (FL, FR, RL, RR, Spare)
- **Pressure and temperature monitoring** with configurable thresholds
- **Battery level tracking** for each sensor
- **Alert system** for low pressure, high temperature, and low battery

### 🔋 **Power Management**
- **Dual battery system** (house battery + starter battery)
- **Automatic switching** based on voltage levels
- **Engine state detection** via GPIO
- **Power-saving modes** when engine is off
- **Real-time voltage and current monitoring**
- **System LED status** indicating power state

### 📡 **GPS and Route Tracking**
- **UART GPS module** support
- **Route history** with waypoint storage
- **Speed and altitude monitoring**
- **Engine state correlation** with GPS data
- **Export capabilities** (CSV, JSON, GPX, KML)

### 🌡️ **Environmental Monitoring**
- **Temperature and humidity sensors** (DHT22)
- **Multiple locations** (outside, cabin, refrigerator)
- **Configurable update intervals**
- **Historical data logging**

### 📊 **Data Management**
- **SQLite database** for local storage
- **Real-time data streaming** via WebSocket
- **REST API** for external access
- **Data export** in multiple formats
- **Automatic cleanup** of old data

## Hardware Requirements

### Raspberry Pi Zero
- **Model**: Raspberry Pi Zero W or Zero 2 W
- **OS**: Raspberry Pi OS Lite (recommended)
- **Storage**: 8GB+ microSD card
- **Power**: 5V/2.5A via micro USB

### Sensors and Modules
- **CC1101 Radio Module**: For TPMS communication
- **DS18B20 Temperature Sensor**: For engine cooling monitoring
- **ADS1115 ADC**: For voltage and current monitoring
- **MPU6050**: For inclinometer data
- **DHT22**: For environmental monitoring
- **GPS Module**: UART-based (e.g., NEO-6M)

### GPIO Pin Assignments
```
Power Management:
- GPIO 18: Power source switch (house/starter battery)
- GPIO 23: Engine state detection
- GPIO 24: System status LED

Cooling System:
- GPIO 5: Low speed fan control
- GPIO 6: High speed fan control
- GPIO 17: Temperature sensor (DS18B20)

TPMS:
- GPIO 8: CC1101 CS (SPI0 CE0)
- GPIO 25: CC1101 GDO0 (interrupt)
- GPIO 26: CC1101 GDO2 (data)

SPI/I2C:
- SPI0: CC1101 radio module
- I2C1: ADS1115 ADC, MPU6050, other I2C sensors
```

## Installation

### 1. System Setup
```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install required packages
sudo apt install -y python3-pip python3-venv git

# Enable SPI and I2C
sudo raspi-config
# Navigate to: Interface Options > SPI > Enable
# Navigate to: Interface Options > I2C > Enable

# Reboot
sudo reboot
```

### 2. Python Environment
```bash
# Create virtual environment
python3 -m venv venv
source venv/bin/activate

# Install dependencies
pip install -r requirements.txt
```

### 3. Hardware Setup
```bash
# Check SPI and I2C are enabled
ls /dev/spi* /dev/i2c*

# Check GPIO access
sudo usermod -a -G gpio $USER
# Log out and back in
```

### 4. Configuration
```bash
# Copy and edit configuration
cp config.yaml.example config.yaml
nano config.yaml

# Set your specific pin assignments and sensor configurations
```

## Usage

### Starting the Sensor Hub
```bash
# Activate virtual environment
source venv/bin/activate

# Run the sensor hub
python src/main.py

# Or run as a service
sudo systemctl start jeep-sensor-hub
sudo systemctl enable jeep-sensor-hub
```

### Testing Components
```bash
# Test all components
python test_components.py

# Test individual components
python -c "from src.sensors.cooling_manager import CoolingManager; print('Cooling manager OK')"
```

### API Access
```bash
# Check status
curl http://localhost:8000/

# Get cooling system status
curl http://localhost:8000/api/cooling/status

# Get TPMS data
curl http://localhost:8000/api/sensors/current

# View API documentation
# Open http://localhost:8000/docs in your browser
```

## Configuration

### Cooling System
```yaml
cooling:
enabled: true
low_speed_trigger_temp: 93    # °C
high_speed_trigger_temp: 105  # °C
min_temp: 60.0                # °C
max_temp: 121.11              # °C
overheat_temp: 112            # °C
operating_temp: 90.5          # °C
temp_update_interval: 1000    # milliseconds
fan_update_interval: 100      # milliseconds
ambient_temp: 25.0            # °C
cooling_efficiency: 0.8
ac_integration: true
manual_override: true
low_speed_pin: 5
high_speed_pin: 6
temp_sensor_pin: 17
temp_sensor_type: "DS18B20"
```

### TPMS Configuration
```yaml
tpms:
enabled: true
spi_bus: 0
spi_device: 0
cs_pin: 8
gdo0_pin: 25
gdo2_pin: 26
frequency: 433.92  # MHz
power: 10          # dBm
sensors:
- id: "0x14AC091"
	name: "Front Left"
	position: "FL"
	low_threshold: 20
	high_threshold: 40
```

### Power Management
```yaml
power_management:
enabled: true
sleep_when_engine_off: true
reduced_polling_when_off: true
wake_on_engine_start: true
battery_monitoring: true
low_battery_threshold: 11.5      # volts
critical_battery_threshold: 10.5 # volts
power_source_switch_pin: 18
engine_signal_pin: 23
system_led_pin: 24
```

## Development

### Project Structure
```
sensor-hub/
├── src/
│   ├── database/          # Database management
│   ├── sensors/           # Sensor interfaces
│   │   ├── cooling_manager.py
│   │   ├── tpms_manager.py
│   │   └── ...
│   ├── communication/     # Serial, WiFi, WebSocket
│   ├── power_management/  # Power and battery management
│   └── main.py           # Main application
├── config.yaml           # Configuration file
├── requirements.txt      # Python dependencies
├── test_components.py   # Component testing
└── README.md            # This file
```

### Adding New Sensors
1. Create a new sensor manager in `src/sensors/`
2. Implement the required interface methods
3. Add configuration to `config.yaml`
4. Update `main.py` to initialize the new sensor
5. Add API endpoints if needed

### Testing
```bash
# Run all tests
python test_components.py

# Run specific tests
python -m pytest tests/

# Check code quality
flake8 src/
black src/
```

## Troubleshooting

### Common Issues

#### GPIO Permission Errors
```bash
# Add user to gpio group
sudo usermod -a -G gpio $USER

# Check GPIO access
groups $USER
ls -la /dev/gpiomem
```

#### SPI/I2C Not Working
```bash
# Check if interfaces are enabled
ls /dev/spi* /dev/i2c*

# Enable in raspi-config
sudo raspi-config
# Interface Options > SPI > Enable
# Interface Options > I2C > Enable
```

#### Sensor Not Responding
```bash
# Check wiring and connections
# Verify power supply
# Check sensor addresses
# Test with i2cdetect or similar tools
```

#### Database Errors
```bash
# Check file permissions
sudo chown pi:pi /home/pi/sensor_data.db
sudo chmod 644 /home/pi/sensor_data.db

# Check disk space
df -h
```

### Logs
```bash
# View system logs
sudo journalctl -u jeep-sensor-hub -f

# View application logs
tail -f /var/log/jeep-sensor-hub.log

# Check system status
sudo systemctl status jeep-sensor-hub
```

## API Reference

### Cooling System Endpoints
- `GET /api/cooling/status` - Get current cooling status
- `GET /api/cooling/config` - Get cooling configuration
- `POST /api/cooling/config` - Update cooling configuration
- `POST /api/cooling/ac` - Set A/C state
- `POST /api/cooling/override` - Set manual fan overrides
- `GET /api/cooling/history` - Get cooling history

### Sensor Endpoints
- `GET /api/sensors/current` - Get all current sensor readings
- `GET /api/history/{sensor_type}` - Get sensor history
- `POST /api/export/{sensor_type}` - Export sensor data

### System Endpoints
- `GET /` - Service information
- `POST /api/engine/toggle` - Toggle engine state
- `GET /docs` - Interactive API documentation

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Support

For issues and questions:
1. Check the troubleshooting section
2. Search existing issues
3. Create a new issue with detailed information
4. Include logs and configuration details

## Roadmap

- [ ] WebSocket real-time data streaming
- [ ] Advanced route analytics
- [ ] Machine learning for predictive maintenance
- [ ] Mobile app integration
- [ ] Cloud data synchronization
- [ ] Advanced power management algorithms
