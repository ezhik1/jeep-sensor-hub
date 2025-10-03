# Environmental Monitoring System

## Overview

The Environmental Monitoring System provides comprehensive temperature and humidity monitoring for the Jeep Sensor Hub using DHT22/DHT11 sensors. It monitors three key locations:

- **Cabin**: Interior temperature and humidity for passenger comfort
- **Outdoor**: External environmental conditions
- **Refrigerator**: Cooler/fridge temperature monitoring

## Features

### Core Capabilities
- **Real-time Monitoring**: Continuous sensor reading with configurable intervals
- **Multi-location Support**: Monitor multiple sensors simultaneously
- **Data Validation**: Automatic validation against configurable thresholds
- **Alert System**: Configurable alerts for out-of-range conditions
- **Historical Data**: Store and retrieve sensor readings with configurable retention
- **Calibration Support**: Individual sensor calibration offsets
- **Error Handling**: Robust error handling with sensor health monitoring

### Sensor Support
- **DHT22**: High-precision temperature (±0.5°C) and humidity (±2-5%) sensors
- **DHT11**: Standard temperature (±2°C) and humidity (±5%) sensors
- **GPIO Integration**: Direct GPIO pin connection to Raspberry Pi Zero
- **I2C Alternative**: Support for I2C-based sensors (future enhancement)

## Architecture

### Components

```
EnvironmentalManager
├── Sensor Configuration
├── Data Collection
├── Validation Engine
├── Alert System
├── Data Storage
└── Health Monitoring
```

### Data Flow

1. **Configuration**: Load sensor settings from YAML config
2. **Initialization**: Setup GPIO pins and sensor objects
3. **Monitoring Loop**: Continuous async sensor reading
4. **Data Processing**: Validation, calibration, and storage
5. **Alert Generation**: Threshold checking and alert creation
6. **Data Retrieval**: API for accessing current and historical data

## Configuration

### YAML Configuration

```yaml
environmental:
  enabled: true
  sensors:
    outdoor:
      type: "DHT22"
      pin: 17  # GPIO17
      update_interval: 5000  # milliseconds
      calibration_offset_temp: -1.0
      calibration_offset_humidity: 0.0
    cabin:
      type: "DHT22"
      pin: 27  # GPIO27
      update_interval: 5000
      calibration_offset_temp: 0.5
      calibration_offset_humidity: -2.0
    refrigerator:
      type: "DHT22"
      pin: 22  # GPIO22
      update_interval: 10000  # Longer for fridge
      calibration_offset_temp: 0.0
      calibration_offset_humidity: 0.0
```

### Configuration Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `enabled` | boolean | `true` | Enable/disable environmental monitoring |
| `type` | string | `"DHT22"` | Sensor type (DHT22 or DHT11) |
| `pin` | integer | Required | GPIO pin number for sensor |
| `update_interval` | integer | `5000` | Reading interval in milliseconds |
| `calibration_offset_temp` | float | `0.0` | Temperature calibration offset in °C |
| `calibration_offset_humidity` | float | `0.0` | Humidity calibration offset in % |

## Usage

### Basic Usage

```python
from src.sensors.environmental_manager_dev import EnvironmentalManagerDev

# Initialize with configuration
env_manager = EnvironmentalManagerDev(config['environmental'])

# Start monitoring
await env_manager.start_monitoring()

# Get current readings
readings = env_manager.get_current_readings()
for location, reading in readings.items():
    if reading:
        print(f"{location}: {reading.temperature:.1f}°C, {reading.humidity:.1f}%")

# Stop monitoring
await env_manager.stop_monitoring()
```

### Advanced Usage

```python
# Get sensor status
status = env_manager.get_sensor_status()
print(f"Cabin sensor: {status['cabin']['status']}")

# Get historical data
readings = env_manager.get_sensor_readings('cabin', limit=100)
for reading in readings:
    print(f"{reading.timestamp}: {reading.temperature}°C")

# Modify alert thresholds
env_manager.set_alert_thresholds('cabin', temp_min=20.0, temp_max=30.0)

# Get statistics
stats = env_manager.get_statistics()
print(f"Total readings: {stats['readings_total']}")
```

## API Reference

### EnvironmentalManagerDev Class

#### Constructor
```python
__init__(self, config: Dict)
```
Initialize the environmental manager with configuration.

#### Methods

##### Monitoring Control
- `start_monitoring()`: Start continuous sensor monitoring
- `stop_monitoring()`: Stop sensor monitoring
- `cleanup()`: Cleanup resources

##### Data Retrieval
- `get_current_readings()`: Get latest readings from all sensors
- `get_sensor_readings(location, limit)`: Get historical readings from specific sensor
- `get_all_readings(limit)`: Get historical readings from all sensors

##### Status and Statistics
- `get_sensor_status()`: Get health status of all sensors
- `get_statistics()`: Get monitoring statistics

##### Configuration
- `set_alert_thresholds(location, **kwargs)`: Modify alert thresholds
- `enable_sensor(location)`: Enable specific sensor
- `disable_sensor(location)`: Disable specific sensor

## Alert System

### Default Thresholds

| Location | Temperature Range | Humidity Range |
|----------|-------------------|----------------|
| Cabin | 15°C - 35°C | 30% - 70% |
| Outdoor | -20°C - 50°C | 10% - 95% |
| Refrigerator | 2°C - 8°C | 20% - 60% |

### Alert Types

- **Low Temperature**: Below minimum threshold
- **High Temperature**: Above maximum threshold
- **Low Humidity**: Below minimum threshold
- **High Humidity**: Above maximum threshold
- **Invalid Reading**: Sensor reading validation failure

### Customizing Alerts

```python
# Set custom thresholds for cabin
env_manager.set_alert_thresholds(
    'cabin',
    temp_min=18.0,      # 18°C minimum
    temp_max=28.0,      # 28°C maximum
    humidity_min=35.0,  # 35% minimum
    humidity_max=65.0   # 65% maximum
)
```

## Data Models

### SensorReading Dataclass

```python
@dataclass
class SensorReading:
    timestamp: datetime      # Reading timestamp
    temperature: float       # Temperature in Celsius
    humidity: float          # Humidity percentage
    location: str           # Sensor location
    sensor_id: str          # Unique sensor identifier
    valid: bool = True      # Reading validity flag
```

### SensorConfig Dataclass

```python
@dataclass
class SensorConfig:
    location: str           # Sensor location
    pin: int               # GPIO pin number
    sensor_type: str       # Sensor type (DHT22/DHT11)
    update_interval: int   # Update interval in milliseconds
    enabled: bool = True   # Sensor enabled flag
    calibration_offset_temp: float = 0.0      # Temperature calibration
    calibration_offset_humidity: float = 0.0  # Humidity calibration
```

## Development and Testing

### Development Version

The `EnvironmentalManagerDev` class provides a development version that:
- Simulates sensor readings without hardware
- Generates realistic temperature and humidity patterns
- Includes time-based variations and trends
- Simulates occasional sensor failures
- Allows testing without physical sensors

### Testing

```bash
# Run the development test
cd sensor-hub
python test_environmental_dev.py

# Run the integration example
python example_integration.py
```

### Simulation Features

- **Realistic Patterns**: Day/night temperature variations
- **Trend Simulation**: Gradual temperature changes over time
- **Failure Simulation**: 5% chance of sensor reading failure
- **Calibration Testing**: Test offset and calibration features

## Hardware Requirements

### Required Components
- **Raspberry Pi Zero**: Main processing unit
- **DHT22/DHT11 Sensors**: Temperature and humidity sensors
- **GPIO Connections**: Direct pin connections
- **Power Supply**: 3.3V power for sensors

### Pin Connections

| Sensor | GPIO Pin | Purpose |
|--------|----------|---------|
| Outdoor | GPIO17 | Temperature/Humidity |
| Cabin | GPIO27 | Temperature/Humidity |
| Refrigerator | GPIO22 | Temperature/Humidity |

### Wiring Diagram

```
DHT22 Sensor
├── VCC → 3.3V
├── GND → Ground
└── DATA → GPIO Pin (17/22/27)
```

## Performance Characteristics

### Reading Intervals
- **Cabin**: 5 seconds (frequent updates for comfort)
- **Outdoor**: 5 seconds (environmental monitoring)
- **Refrigerator**: 10 seconds (stable temperature, less frequent)

### Data Retention
- **Memory Storage**: Last 1000 readings per sensor
- **Database Storage**: Configurable retention period
- **Real-time Access**: Current readings always available

### Error Handling
- **Sensor Failures**: Automatic error counting and status tracking
- **Invalid Readings**: Validation and alert generation
- **Connection Issues**: Robust error recovery

## Integration

### With Main Sensor Hub

```python
# In main sensor hub application
from src.sensors import EnvironmentalManagerDev

class SensorHub:
    def __init__(self, config):
        self.environmental = EnvironmentalManagerDev(config['environmental'])

    async def start(self):
        await self.environmental.start_monitoring()

    async def stop(self):
        await self.environmental.stop_monitoring()
```

### With Web API

```python
# Expose environmental data via API
@app.get("/api/environmental/current")
async def get_current_readings():
    return env_manager.get_current_readings()

@app.get("/api/environmental/status")
async def get_sensor_status():
    return env_manager.get_sensor_status()
```

### With ESP32 Displays

Environmental data can be transmitted to ESP32 displays via:
- **Ethernet**: High-speed TCP/IP communication
- **UART**: Serial communication as fallback
- **WebSocket**: Real-time updates

## Troubleshooting

### Common Issues

1. **Sensor Not Reading**
   - Check GPIO pin configuration
   - Verify power supply (3.3V)
   - Check wiring connections

2. **Invalid Readings**
   - Verify sensor type in configuration
   - Check calibration offsets
   - Validate threshold settings

3. **High Error Count**
   - Check sensor health
   - Verify update intervals
   - Check for interference

### Debug Mode

Enable debug logging for detailed information:

```python
import logging
logging.getLogger('src.sensors.environmental_manager_dev').setLevel(logging.DEBUG)
```

## Future Enhancements

### Planned Features
- **I2C Sensor Support**: Additional sensor types
- **Database Integration**: Persistent storage
- **Web Dashboard**: Real-time monitoring interface
- **Alert Notifications**: Email/SMS alerts
- **Data Export**: CSV/JSON data export
- **Machine Learning**: Predictive temperature modeling

### Extensibility
The system is designed for easy extension:
- Add new sensor types
- Implement custom validation rules
- Create specialized alert handlers
- Integrate with external systems

## Support

For issues and questions:
1. Check the troubleshooting section
2. Review configuration settings
3. Enable debug logging
4. Check hardware connections
5. Verify sensor compatibility

## License

This environmental monitoring system is part of the Jeep Sensor Hub project and follows the same licensing terms.
