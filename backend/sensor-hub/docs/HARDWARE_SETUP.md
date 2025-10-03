# DHT Sensor Hardware Setup Guide

## Overview

This guide covers the hardware setup for connecting DHT22/DHT11 temperature and humidity sensors to your Raspberry Pi Zero for the Jeep Sensor Hub environmental monitoring system.

## Required Components

### Hardware
- **Raspberry Pi Zero** (or Pi Zero W)
- **3x DHT22 sensors** (recommended) or DHT11 sensors
- **Breadboard** (optional, for prototyping)
- **Jumper wires** (male-to-female and male-to-male)
- **3.3V power supply** (Pi Zero provides this)
- **4.7kΩ pull-up resistors** (3x, one per sensor)

### Software
- **Raspberry Pi OS** (Raspbian)
- **Python 3.7+**
- **Required Python packages** (see installation section)

## Pin Assignments

| Sensor Location | GPIO Pin | Purpose | Wire Color (suggested) |
|-----------------|----------|---------|------------------------|
| **Cabin** | GPIO17 | Temperature/Humidity | Red |
| **Outdoor** | GPIO18 | Temperature/Humidity | Yellow |
| **Refrigerator** | GPIO19 | Temperature/Humidity | Blue |

## Wiring Diagram

```
DHT22 Sensor (Cabin - GPIO17)
├── VCC (Pin 1) → 3.3V (Pin 1 or 17)
├── DATA (Pin 2) → GPIO17 (Pin 11) + 4.7kΩ pull-up to 3.3V
└── GND (Pin 4) → Ground (Pin 6, 9, 14, 20, 25, 30, 34, or 39)

DHT22 Sensor (Outdoor - GPIO18)
├── VCC (Pin 1) → 3.3V (Pin 1 or 17)
├── DATA (Pin 2) → GPIO18 (Pin 12) + 4.7kΩ pull-up to 3.3V
└── GND (Pin 4) → Ground (Pin 6, 9, 14, 20, 25, 30, 34, or 39)

DHT22 Sensor (Refrigerator - GPIO19)
├── VCC (Pin 1) → 3.3V (Pin 1 or 17)
├── DATA (Pin 2) → GPIO19 (Pin 35) + 4.7kΩ pull-up to 3.3V
└── GND (Pin 4) → Ground (Pin 6, 9, 14, 20, 25, 30, 34, or 39)
```

## Detailed Wiring Instructions

### Step 1: Power Connections
1. **3.3V Power**: Connect all three sensors' VCC pins to the Pi Zero's 3.3V pin
2. **Ground**: Connect all three sensors' GND pins to the Pi Zero's ground pins

### Step 2: Data Connections
1. **Cabin Sensor**: Connect DATA pin to GPIO17 (Pin 11)
2. **Outdoor Sensor**: Connect DATA pin to GPIO18 (Pin 12)
3. **Refrigerator Sensor**: Connect DATA pin to GPIO19 (Pin 35)

### Step 3: Pull-up Resistors
1. **4.7kΩ Resistors**: Connect one end of each resistor to 3.3V
2. **Other End**: Connect to the respective DATA pin of each sensor
3. **Purpose**: Ensures stable signal levels and prevents floating inputs

## Breadboard Layout (Optional)

```
Power Rail (3.3V)
├── 4.7kΩ → DHT22 Cabin DATA
├── 4.7kΩ → DHT22 Outdoor DATA
└── 4.7kΩ → DHT22 Refrigerator DATA

Ground Rail
├── DHT22 Cabin GND
├── DHT22 Outdoor GND
└── DHT22 Refrigerator GND

Signal Connections
├── GPIO17 → DHT22 Cabin DATA
├── GPIO18 → DHT22 Outdoor DATA
└── GPIO19 → DHT22 Refrigerator DATA
```

## Physical Installation

### Indoor Sensors (Cabin & Refrigerator)
1. **Mounting**: Use adhesive or small brackets to secure sensors
2. **Location**: Place in areas with good air circulation
3. **Protection**: Avoid direct sunlight, heat sources, or moisture
4. **Cable Management**: Secure cables to prevent damage

### Outdoor Sensor
1. **Weatherproofing**: Use a weatherproof enclosure
2. **Mounting**: Secure to vehicle exterior (bumper, roof rack, etc.)
3. **Cable Routing**: Route cables through existing grommets or create new ones
4. **Protection**: Ensure sensor is protected from direct rain/snow

## Testing the Setup

### 1. Basic Connection Test
```bash
# Test GPIO access
sudo python3 -c "import RPi.GPIO as GPIO; GPIO.setmode(GPIO.BCM); print('GPIO access OK')"
```

### 2. Individual Sensor Test
```bash
# Test single DHT sensor
cd sensor-hub
python3 test_environmental_production.py
```

### 3. Full System Test
```bash
# Test complete environmental monitoring
python3 example_integration.py
```

## Troubleshooting

### Common Issues

#### 1. "No module named 'Adafruit_DHT'"
```bash
# Install the DHT library
pip3 install Adafruit_DHT
```

#### 2. "GPIO access denied"
```bash
# Run with sudo or add user to gpio group
sudo usermod -a -G gpio $USER
# Then logout and login again
```

#### 3. "Sensor not responding"
- Check power connections (3.3V, not 5V)
- Verify ground connections
- Check data pin connections
- Ensure pull-up resistors are connected
- Test with multimeter for continuity

#### 4. "Invalid readings"
- Check sensor type in configuration (DHT22 vs DHT11)
- Verify calibration offsets
- Check for electrical interference
- Ensure proper cable routing

#### 5. "High error count"
- Check sensor health and connections
- Verify update intervals aren't too fast
- Check for power supply issues
- Test individual sensors

### Debug Commands

```bash
# Check GPIO status
gpio readall

# Monitor GPIO in real-time
watch -n 1 'gpio readall'

# Check system logs
sudo journalctl -f

# Test specific GPIO pin
sudo python3 -c "
import RPi.GPIO as GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setup(17, GPIO.IN, pull_up_down=GPIO.PUD_UP)
print('GPIO17 state:', GPIO.input(17))
"
```

## Performance Optimization

### Reading Intervals
- **Cabin**: 5 seconds (comfort monitoring)
- **Outdoor**: 5 seconds (environmental monitoring)
- **Refrigerator**: 10 seconds (stable temperature)

### Power Considerations
- DHT22 sensors use ~2.5mA each
- Total power draw: ~7.5mA
- Pi Zero can easily handle this load

### Signal Quality
- Keep sensor cables short (< 1m recommended)
- Avoid running near high-voltage cables
- Use shielded cables if available
- Ensure proper grounding

## Safety Considerations

### Electrical Safety
- **Never connect 5V to DHT sensors** (they operate on 3.3V)
- **Use proper grounding** to prevent electrical noise
- **Check connections** before powering on
- **Disconnect power** before making changes

### Environmental Safety
- **Protect outdoor sensors** from weather
- **Secure cables** to prevent damage
- **Avoid extreme temperatures** beyond sensor specifications
- **Regular inspection** for wear and damage

## Maintenance

### Regular Checks
- **Monthly**: Inspect connections and cables
- **Quarterly**: Clean sensor surfaces
- **Annually**: Check for corrosion or damage

### Calibration
- **Initial**: Set calibration offsets based on known good readings
- **Periodic**: Verify accuracy against reference thermometer/hygrometer
- **Adjustment**: Update calibration offsets in configuration

## Advanced Configuration

### Custom Pin Assignments
```yaml
environmental:
  sensors:
    custom_location:
      pin: 23  # Different GPIO pin
      type: "DHT11"  # Different sensor type
      update_interval: 3000  # Custom interval
```

### Multiple Sensors per Location
```yaml
environmental:
  sensors:
    cabin_front:
      pin: 17
      type: "DHT22"
    cabin_rear:
      pin: 27
      type: "DHT22"
```

### Sensor-Specific Settings
```yaml
environmental:
  sensors:
    outdoor:
      pin: 18
      type: "DHT22"
      calibration_offset_temp: -1.5  # Compensate for heat
      calibration_offset_humidity: 2.0  # Compensate for enclosure
```

## Support and Resources

### Documentation
- [DHT22 Datasheet](https://www.adafruit.com/product/385)
- [Raspberry Pi GPIO Documentation](https://www.raspberrypi.org/documentation/usage/gpio/)
- [Adafruit DHT Library](https://github.com/adafruit/Adafruit_Python_DHT)

### Community Support
- Raspberry Pi Forums
- Adafruit Forums
- GitHub Issues

### Professional Help
- Local electronics shops
- Raspberry Pi specialists
- IoT consultants

## Next Steps

After completing the hardware setup:

1. **Test individual sensors** using the test scripts
2. **Verify configuration** in `config.yaml`
3. **Run full system test** with `example_integration.py`
4. **Integrate with main sensor hub** application
5. **Configure monitoring and alerts**
6. **Deploy to production environment**

## Conclusion

Proper hardware setup is crucial for reliable environmental monitoring. Follow this guide carefully, test thoroughly, and maintain your sensors regularly for optimal performance.

For additional support or questions, refer to the main project documentation or create an issue in the project repository.
