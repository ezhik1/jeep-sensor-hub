#!/bin/bash
# DHT Sensor Library Installation Script for Raspberry Pi
# This script installs the required dependencies for the environmental monitoring system

echo "=========================================="
echo "DHT Sensor Library Installation Script"
echo "=========================================="
echo ""

# Check if running on Raspberry Pi
if ! grep -q "Raspberry Pi" /proc/cpuinfo 2>/dev/null; then
    echo "⚠️  Warning: This script is designed for Raspberry Pi"
    echo "   It may not work correctly on other systems"
    echo ""
fi

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "❌ This script must be run as root (use sudo)"
    echo "   Run: sudo ./install_dht.sh"
    exit 1
fi

echo "🔧 Installing DHT sensor libraries and dependencies..."
echo ""

# Update package list
echo "📦 Updating package list..."
apt update

# Install system dependencies
echo "📦 Installing system dependencies..."
apt install -y python3-pip python3-dev python3-setuptools

# Install GPIO library
echo "📦 Installing RPi.GPIO..."
pip3 install RPi.GPIO

# Install DHT library (Adafruit version)
echo "📦 Installing Adafruit DHT library..."
pip3 install Adafruit_DHT

# Install alternative DHT library (CircuitPython version)
echo "📦 Installing CircuitPython DHT library..."
pip3 install adafruit-circuitpython-dht

# Install additional dependencies
echo "📦 Installing additional dependencies..."
pip3 install pyyaml typing-extensions

echo ""
echo "✅ Installation completed!"
echo ""

# Test the installation
echo "🧪 Testing installation..."
echo ""

# Test GPIO access
echo "Testing GPIO access..."
if python3 -c "import RPi.GPIO as GPIO; GPIO.setmode(GPIO.BCM); print('✅ GPIO access OK')" 2>/dev/null; then
    echo "✅ GPIO library working correctly"
else
    echo "❌ GPIO library test failed"
fi

# Test DHT library
echo "Testing DHT library..."
if python3 -c "import Adafruit_DHT; print('✅ Adafruit DHT library OK')" 2>/dev/null; then
    echo "✅ Adafruit DHT library working correctly"
else
    echo "❌ Adafruit DHT library test failed"
fi

# Test CircuitPython DHT library
echo "Testing CircuitPython DHT library..."
if python3 -c "import adafruit_dht; print('✅ CircuitPython DHT library OK')" 2>/dev/null; then
    echo "✅ CircuitPython DHT library working correctly"
else
    echo "❌ CircuitPython DHT library test failed"
fi

echo ""
echo "=========================================="
echo "Installation Summary"
echo "=========================================="
echo ""

# Check installed packages
echo "📋 Installed packages:"
pip3 list | grep -E "(RPi|Adafruit|adafruit-circuitpython)" | sort

echo ""
echo "🎯 Next steps:"
echo "1. Connect your DHT sensors to the GPIO pins"
echo "2. Update your config.yaml with sensor settings"
echo "3. Test with: python3 test_environmental_production.py"
echo "4. Run full system: python3 example_integration.py"
echo ""

echo "📚 For hardware setup, see: docs/HARDWARE_SETUP.md"
echo "📖 For usage examples, see: docs/ENVIRONMENTAL_MONITORING.md"
echo ""

echo "✅ Installation script completed successfully!"
echo "   Your Raspberry Pi is now ready for DHT sensor monitoring!"
