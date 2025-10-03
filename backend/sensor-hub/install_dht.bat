@echo off
REM DHT Sensor Library Installation Script for Windows Development
REM This script installs the required dependencies for development and testing

echo ==========================================
echo DHT Sensor Library Installation Script
echo ==========================================
echo.

echo Note: This is a Windows development environment
echo For production use, run install_dht.sh on Raspberry Pi
echo.

echo Installing Python dependencies for development...
echo.

REM Check if Python is available
python --version >nul 2>&1
if errorlevel 1 (
    echo Error: Python not found in PATH
    echo Please install Python 3.7+ and add it to PATH
    pause
    exit /b 1
)

echo Python found:
python --version

echo.
echo Installing development dependencies...
echo.

REM Install basic dependencies
echo Installing pyyaml...
pip install pyyaml

echo Installing typing-extensions...
pip install typing-extensions

echo.
echo Development dependencies installed!
echo.

echo Note: DHT libraries (Adafruit_DHT, RPi.GPIO) are not available on Windows
echo These will only work on Raspberry Pi hardware
echo.

echo To test the development version:
echo 1. Run: python test_environmental_dev.py
echo 2. This uses simulated sensors for development
echo.

echo For production deployment on Raspberry Pi:
echo 1. Copy files to Raspberry Pi
echo 2. Run: sudo ./install_dht.sh
echo 3. Test with: python3 test_environmental_production.py
echo.

echo Installation script completed!
pause
