# Jeep Sensor Hub User Guide

Welcome to the comprehensive user guide for the Jeep Sensor Hub system. This guide covers all aspects of using and operating the vehicle monitoring and control system.

## Table of Contents

1. [Getting Started](#getting-started)
2. [System Overview](#system-overview)
3. [Display Modules](#display-modules)
4. [Web Interface](#web-interface)
5. [Sensor Monitoring](#sensor-monitoring)
6. [GPS Route Tracking](#gps-route-tracking)
7. [Power Management](#power-management)
8. [Cooling System](#cooling-system)
9. [Data Export](#data-export)
10. [Troubleshooting](#troubleshooting)
11. [Maintenance](#maintenance)
12. [FAQ](#faq)

## Getting Started

### First Time Setup

1. **Power On**: Ensure the auxiliary battery is connected and the system is powered
2. **Display Initialization**: Wait for all three display modules to boot up
3. **WiFi Connection**: Connect to the sensor hub's WiFi network
4. **Web Interface**: Open a web browser and navigate to the hub's IP address
5. **Initial Configuration**: Set up basic preferences and sensor calibrations

### System Requirements

- **Vehicle**: Jeep Wrangler or compatible 4x4 vehicle
- **Power**: 12V auxiliary battery system
- **Connectivity**: WiFi-capable device for web interface access
- **Browser**: Modern web browser (Chrome, Firefox, Safari, Edge)

## System Overview

### What the System Monitors

The Jeep Sensor Hub continuously monitors:
- **Engine Temperature**: Coolant and oil temperature
- **Environmental Conditions**: Cabin and ambient temperature/humidity
- **Tire Pressure**: TPMS sensor data from all wheels
- **Power System**: Battery voltage, current, and health
- **GPS Location**: Position, speed, altitude, and route tracking
- **Vehicle Status**: Engine on/off, A/C state, and system health

### System Components

- **Main Hub**: Raspberry Pi Zero mounted in the engine compartment
- **Dashboard Display**: 3" OLED screen with full control access
- **Secondary Display**: Identical display mounted in the trunk
- **Remote Display**: Battery-powered portable display
- **Web Interface**: Full-featured dashboard accessible from any device

## Display Modules

### Dashboard Display (Primary)

The main dashboard display provides:
- **Real-time Data**: Current sensor readings and system status
- **Quick Controls**: Fan override, A/C control, and power switching
- **Navigation**: Menu system for accessing all features
- **Alerts**: Visual and audible notifications for critical conditions

#### Navigation Controls

- **Menu Button**: Access main menu and settings
- **Select Button**: Confirm selections and enter submenus
- **Back Button**: Return to previous menu level
- **Up/Down Buttons**: Navigate through menu options
- **Enter Button**: Activate selected function

#### Display Layouts

1. **Main Dashboard**: Overview of all critical systems
2. **Sensor Detail**: Detailed view of specific sensor data
3. **GPS Map**: Current location and route information
4. **Power Status**: Battery levels and power consumption
5. **Cooling System**: Engine temperature and fan status
6. **Settings**: System configuration and preferences

### Secondary Display (Trunk)

The trunk-mounted display provides:
- **Identical Functionality**: Same features as dashboard display
- **Remote Monitoring**: Monitor system from rear of vehicle
- **Passenger Access**: Allow passengers to view system status
- **Backup Interface**: Redundant control if dashboard display fails

### Remote Display (Portable)

The portable remote display offers:
- **Read-only Access**: View system status without control
- **Battery Power**: Independent power source for portability
- **Wireless Connection**: WiFi connection to main hub
- **Compact Design**: Easy to carry and position anywhere

## Web Interface

### Accessing the Interface

1. **Connect to Hub**: Join the sensor hub's WiFi network
2. **Open Browser**: Navigate to the hub's IP address
3. **Login**: Enter credentials if authentication is enabled
4. **Dashboard**: Access the main monitoring interface

### Main Dashboard

The web dashboard provides:
- **Real-time Monitoring**: Live sensor data updates
- **Interactive Charts**: Historical data visualization
- **GPS Maps**: Route tracking and location history
- **System Control**: Remote control of all functions
- **Data Export**: Download sensor data and reports

### Key Features

- **Responsive Design**: Works on desktop, tablet, and mobile
- **Real-time Updates**: WebSocket connection for live data
- **Interactive Elements**: Clickable charts and controls
- **Data Filtering**: Time-based and sensor-based filtering
- **Export Options**: Multiple format support (CSV, JSON, GPX)

## Sensor Monitoring

### Temperature Sensors

#### Engine Temperature
- **Normal Range**: 180°F - 220°F (82°C - 104°C)
- **Warning Level**: 220°F - 240°F (104°C - 116°C)
- **Critical Level**: Above 240°F (116°C)
- **Monitoring**: Continuous with 1-second updates

#### Cabin Temperature
- **Range**: -40°F to 140°F (-40°C to 60°C)
- **Accuracy**: ±2°F (±1°C)
- **Update Rate**: Every 30 seconds
- **Location**: Dashboard area

#### Ambient Temperature
- **Range**: -40°F to 140°F (-40°C to 60°C)
- **Accuracy**: ±2°F (±1°C)
- **Update Rate**: Every 60 seconds
- **Location**: Engine compartment

### Humidity Sensors

#### Cabin Humidity
- **Range**: 0% to 100% RH
- **Accuracy**: ±3% RH
- **Update Rate**: Every 30 seconds
- **Use**: Comfort monitoring and condensation detection

#### Ambient Humidity
- **Range**: 0% to 100% RH
- **Accuracy**: ±3% RH
- **Update Rate**: Every 60 seconds
- **Use**: Environmental monitoring

### Pressure Sensors

#### Tire Pressure (TPMS)
- **Range**: 0 to 100 PSI
- **Accuracy**: ±1 PSI
- **Update Rate**: Every 5 seconds
- **Alerts**: Low pressure warnings and recommendations

#### Oil Pressure
- **Range**: 0 to 100 PSI
- **Normal Range**: 20 to 80 PSI
- **Warning Level**: Below 20 PSI
- **Update Rate**: Every 2 seconds

#### Fuel Pressure
- **Range**: 0 to 100 PSI
- **Normal Range**: 2 to 4 PSI
- **Warning Level**: Below 2 PSI
- **Update Rate**: Every 5 seconds

### Power Sensors

#### Voltage Monitoring
- **House Battery**: 12V system monitoring
- **Starter Battery**: 12V system monitoring
- **Auxiliary**: 5V and 3.3V monitoring
- **Accuracy**: ±0.1V
- **Update Rate**: Every second

#### Current Monitoring
- **House Battery**: 0 to 50A
- **Starter Battery**: 0 to 50A
- **Auxiliary**: 0 to 10A
- **Accuracy**: ±0.1A
- **Update Rate**: Every second

## GPS Route Tracking

### Route Recording

#### Automatic Recording
- **Start Condition**: Engine starts
- **End Condition**: Engine stops
- **Data Collection**: Position, speed, altitude every second
- **Storage**: Local database with backup options

#### Manual Recording
- **Start**: User-initiated route start
- **End**: User-initiated route end
- **Naming**: Custom route names and descriptions
- **Tags**: Categorization and notes

### Route Data

#### Position Information
- **Latitude/Longitude**: GPS coordinates
- **Altitude**: Elevation above sea level
- **Speed**: Ground speed in MPH/KPH
- **Heading**: Direction of travel
- **Accuracy**: GPS signal quality

#### Route Statistics
- **Distance**: Total route distance
- **Duration**: Travel time and stops
- **Average Speed**: Overall and segment averages
- **Elevation**: Gain, loss, and profile
- **Fuel Efficiency**: MPG calculations

### Route Management

#### Viewing Routes
- **List View**: Chronological route listing
- **Map View**: Interactive map with route overlay
- **Statistics**: Summary data and charts
- **Details**: Individual waypoint information

#### Route Export
- **Formats**: GPX, KML, CSV, JSON
- **Time Range**: Custom date selection
- **Route Selection**: Individual or multiple routes
- **Data Types**: Full or summary data

## Power Management

### Battery System

#### Dual Battery Setup
- **House Battery**: Primary power source for accessories
- **Starter Battery**: Engine starting and backup power
- **Automatic Switching**: Seamless power source management
- **Manual Override**: Physical switch for manual control

#### Battery Monitoring
- **Voltage**: Real-time voltage monitoring
- **Health**: Battery condition and age tracking
- **Charging**: Charging status and efficiency
- **Alerts**: Low voltage and health warnings

### Power Optimization

#### Smart Power Management
- **Engine Off Mode**: Reduced sensor polling
- **Sleep Mode**: Minimal power consumption
- **Wake Triggers**: Motion, voltage, or time-based
- **Efficiency Monitoring**: Power consumption tracking

#### Power Saving Features
- **Adaptive Polling**: Adjust sensor update rates
- **Selective Monitoring**: Focus on critical sensors
- **Display Dimming**: Automatic brightness adjustment
- **WiFi Management**: Power-efficient connectivity

## Cooling System

### Fan Control

#### Automatic Operation
- **Low Speed Trigger**: 93°F (34°C)
- **High Speed Trigger**: 105°F (41°C)
- **Smart Buffering**: Predictive cooling activation
- **A/C Integration**: Automatic fan control with air conditioning

#### Manual Control
- **Fan Override**: Manual fan speed control
- **Duration Setting**: Temporary override periods
- **Emergency Mode**: Maximum cooling activation
- **Status Monitoring**: Real-time fan operation status

### Temperature Management

#### Threshold Configuration
- **Low Speed**: Adjustable trigger temperature
- **High Speed**: Adjustable trigger temperature
- **Overheat**: Critical temperature warning
- **Operating**: Optimal temperature range

#### Safety Features
- **Thermal Protection**: Automatic shutdown on overheat
- **Fan Monitoring**: Detection of fan failures
- **Temperature Validation**: Sensor accuracy verification
- **Emergency Procedures**: Fail-safe operation modes

## Data Export

### Export Options

#### Data Types
- **Sensor Data**: All sensor readings and history
- **GPS Routes**: Complete route and waypoint data
- **Power Data**: Battery and consumption information
- **Cooling Data**: Temperature and fan operation history
- **System Logs**: Event logs and system status

#### Export Formats
- **CSV**: Spreadsheet-compatible data
- **JSON**: Structured data for applications
- **GPX**: GPS exchange format for mapping
- **KML**: Google Earth compatible format
- **ZIP**: Compressed archive of all data

### Export Management

#### Scheduling
- **Automatic Export**: Regular data backup
- **Manual Export**: On-demand data download
- **Incremental Export**: Only new or changed data
- **Full Export**: Complete dataset download

#### Data Retention
- **Local Storage**: On-device data retention
- **Backup Options**: External storage and cloud backup
- **Cleanup Policies**: Automatic old data removal
- **Archive Options**: Long-term data preservation

## Troubleshooting

### Common Issues

#### Display Problems
- **No Display**: Check power and connections
- **Frozen Display**: Restart the display module
- **Poor Visibility**: Adjust brightness settings
- **Connection Issues**: Verify WiFi connectivity

#### Sensor Issues
- **Missing Data**: Check sensor connections
- **Inaccurate Readings**: Calibrate or replace sensors
- **Communication Errors**: Verify wiring and protocols
- **Power Issues**: Check sensor power supply

#### System Problems
- **No Response**: Restart the main hub
- **Slow Performance**: Check system resources
- **Connection Loss**: Verify network settings
- **Data Loss**: Check storage and backup status

### Error Messages

#### Understanding Alerts
- **Warning Level**: Yellow indicators for attention
- **Critical Level**: Red indicators for immediate action
- **Information Level**: Blue indicators for status updates
- **Success Level**: Green indicators for normal operation

#### Error Codes
- **E001**: Communication error with sensor
- **E002**: Power supply voltage low
- **E003**: Temperature sensor failure
- **E004**: GPS signal lost
- **E005**: Database connection error

### Recovery Procedures

#### System Restart
1. **Graceful Shutdown**: Use proper shutdown procedure
2. **Power Cycle**: Disconnect and reconnect power
3. **Wait Period**: Allow 30 seconds for restart
4. **Verify Operation**: Check all systems are online

#### Data Recovery
1. **Check Backups**: Verify backup data integrity
2. **Restore Data**: Use backup restoration procedures
3. **Verify Operation**: Confirm system functionality
4. **Update Backups**: Create new backup after recovery

## Maintenance

### Regular Maintenance

#### Monthly Tasks
- **Visual Inspection**: Check all connections and components
- **Data Backup**: Export and backup all data
- **System Updates**: Check for firmware and software updates
- **Performance Review**: Analyze system performance metrics

#### Quarterly Tasks
- **Sensor Calibration**: Verify sensor accuracy
- **Hardware Inspection**: Detailed component examination
- **Software Updates**: Install available updates
- **Data Cleanup**: Remove old and unnecessary data

#### Annual Tasks
- **Full System Test**: Comprehensive functionality testing
- **Component Replacement**: Replace aging components
- **Performance Optimization**: Tune system parameters
- **Documentation Update**: Update system documentation

### Preventive Maintenance

#### Hardware Care
- **Connection Inspection**: Check for loose or corroded connections
- **Component Cleaning**: Remove dust and debris
- **Mounting Verification**: Ensure secure mounting
- **Environmental Protection**: Check weatherproofing

#### Software Maintenance
- **Log Analysis**: Review system logs for issues
- **Performance Monitoring**: Track system performance trends
- **Security Updates**: Install security patches
- **Configuration Review**: Verify optimal settings

## FAQ

### General Questions

**Q: How long does the system run on battery power?**
A: The system can run for 24-48 hours on the auxiliary battery, depending on sensor polling frequency and display usage.

**Q: Can I add more sensors to the system?**
A: Yes, the system is designed to be modular and can accommodate additional sensors through the expansion interfaces.

**Q: Is the system weatherproof?**
A: The main components are designed for automotive use and can handle typical weather conditions, but should be protected from direct water exposure.

**Q: Can I access the system remotely?**
A: Yes, the web interface can be accessed from any device on the same network, and remote access can be configured with proper security measures.

### Technical Questions

**Q: What happens if a sensor fails?**
A: The system will detect the failure, log an error, and continue operating with other sensors. Failed sensors are clearly indicated in the interface.

**Q: How accurate are the temperature readings?**
A: Temperature sensors have an accuracy of ±2°F (±1°C) and are calibrated for automotive use.

**Q: Can I export my data to other applications?**
A: Yes, data can be exported in multiple formats including CSV, JSON, GPX, and KML for use in other applications.

**Q: How often is data saved?**
A: Sensor data is saved every minute, GPS data every second during routes, and system events are logged immediately.

### Troubleshooting Questions

**Q: The display is not showing any data.**
A: Check power connections, verify WiFi connectivity, and restart the display module. If problems persist, restart the main hub.

**Q: GPS routes are not being recorded.**
A: Verify GPS module connection, check for clear sky view, and ensure the engine start/stop detection is working properly.

**Q: Temperature readings seem inaccurate.**
A: Check sensor placement, verify connections, and recalibrate sensors if necessary. Ensure sensors are not exposed to direct heat sources.

**Q: The system is using too much power.**
A: Reduce sensor polling frequency, enable power-saving modes, and check for parasitic loads or malfunctioning components.

## Support and Contact

### Getting Help
- **Documentation**: Check this user guide and technical documentation
- **Online Resources**: Visit the project website and forums
- **Issue Tracking**: Report problems through the issue tracker
- **Community Support**: Connect with other users and developers

### Contact Information
- **Project Lead**: [Contact Information]
- **Technical Support**: [Support Email]
- **Documentation**: [Documentation Link]
- **Issues**: [Issue Tracker Link]

---

**Need help?** Check the troubleshooting section or contact support for assistance.
