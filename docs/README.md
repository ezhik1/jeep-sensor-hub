# Jeep Sensor Hub Documentation

Welcome to the comprehensive documentation for the Jeep Sensor Hub Research project. This documentation covers all aspects of the system from installation and setup to development and troubleshooting.

## Documentation Structure

```
docs/
├── api/                 # API documentation and reference
├── hardware/            # Hardware schematics and specifications
├── installation/        # Installation and setup guides
├── user-guide/          # User manual and operation guides
├── development/         # Development guidelines and API docs
└── README.md            # This file
```

## Quick Start

### For Users
1. **Installation Guide** - [Installation Guide](installation/README.md)
2. **User Manual** - [User Guide](user-guide/README.md)
3. **Hardware Setup** - [Hardware Documentation](../hardware/README.md)

### For Developers
1. **Development Setup** - [Development Guide](development/README.md)
2. **API Reference** - [API Documentation](api/README.md)
3. **Architecture Overview** - [Project Structure](../project-structure.md)

### For System Administrators
1. **Deployment Guide** - [Installation Guide](installation/deployment.md)
2. **Configuration Reference** - [Configuration Guide](installation/configuration.md)
3. **Troubleshooting** - [Troubleshooting Guide](installation/troubleshooting.md)

## System Overview

The Jeep Sensor Hub is a comprehensive vehicle monitoring and control system that integrates:

- **Sensor Monitoring** - Temperature, humidity, TPMS, GPS, power, and more
- **Engine Cooling Management** - Intelligent fan control and temperature monitoring
- **Power Management** - Battery switching and power consumption monitoring
- **Real-time Display** - Multiple ESP32-based display modules
- **Web Interface** - Vue.js frontend with real-time data visualization
- **Backend API** - Node.js/Express middleware for data processing
- **Data Storage** - SQLite database with export capabilities

## Key Features

### 🚗 Vehicle Monitoring
- **Real-time Sensor Data** - Continuous monitoring of all vehicle systems
- **GPS Route Tracking** - Complete journey logging with speed and altitude
- **TPMS Integration** - Tire pressure monitoring and alerts
- **Environmental Monitoring** - Temperature and humidity tracking

### 🌡️ Engine Cooling System
- **Dual-Speed Fan Control** - Intelligent temperature-based operation
- **Smart Buffering** - Predictive cooling to prevent overheating
- **A/C Integration** - Automatic fan control based on air conditioning state
- **Manual Override** - User control for specific situations

### ⚡ Power Management
- **Dual Battery System** - Automatic switching between house and starter batteries
- **Power Monitoring** - Real-time voltage and current tracking
- **Efficiency Optimization** - Power-saving modes when vehicle is off
- **Battery Health** - Long-term battery status monitoring

### 📱 Multi-Platform Interface
- **Dashboard Display** - Main dashboard-mounted screen with full control
- **Secondary Display** - Trunk-mounted screen with identical functionality
- **Remote Display** - Battery-powered remote screen with read-only access
- **Web Interface** - Full-featured web UI accessible from any device

## System Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Web Frontend  │    │  Backend API    │    │  Sensor Hub     │
│   (Vue.js)      │◄──►│  (Node.js)      │◄──►│  (Raspberry Pi) │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                                │                       │
                                ▼                       ▼
                       ┌─────────────────┐    ┌─────────────────┐
                       │   Database      │    │  ESP32 Displays │
                       │   (SQLite)      │    │  (MicroPython)  │
                       └─────────────────┘    └─────────────────┘
```

## Technology Stack

### Frontend
- **Vue.js 3** - Modern reactive framework
- **Pinia** - State management
- **Vue Router** - Client-side routing
- **Chart.js** - Data visualization
- **Leaflet** - Interactive maps

### Backend
- **Node.js** - JavaScript runtime
- **Express.js** - Web framework
- **WebSocket** - Real-time communication
- **SQLite** - Local database
- **Winston** - Logging framework

### Hardware
- **Raspberry Pi Zero** - Main controller (Python)
- **ESP32 Modules** - Display nodes (MicroPython)
- **Various Sensors** - Temperature, GPS, TPMS, etc.
- **Custom PCBs** - Sensor interfaces and power management

## Getting Help

### Documentation Issues
If you find errors, inconsistencies, or missing information in the documentation:
1. Check the [Issues](../../issues) page for existing reports
2. Create a new issue with the `documentation` label
3. Include specific details about what needs to be fixed

### Technical Support
For technical issues or questions:
1. Check the [Troubleshooting Guide](installation/troubleshooting.md)
2. Review the [FAQ](user-guide/faq.md)
3. Search existing [Issues](../../issues)
4. Create a new issue with detailed problem description

### Feature Requests
To request new features or improvements:
1. Check the [Roadmap](development/roadmap.md) for planned features
2. Create a new issue with the `enhancement` label
3. Describe the feature and its benefits

## Contributing

We welcome contributions to improve the documentation:

### How to Contribute
1. **Fork the repository**
2. **Create a feature branch** for your changes
3. **Make your changes** following the style guide
4. **Test your changes** to ensure they work correctly
5. **Submit a pull request** with a clear description

### Documentation Standards
- **Markdown Format** - Use standard Markdown syntax
- **Clear Structure** - Organize content logically with headers
- **Code Examples** - Include working code samples
- **Screenshots** - Add visual aids where helpful
- **Links** - Cross-reference related documentation

### Style Guide
- **Headers** - Use descriptive header names
- **Code Blocks** - Specify language for syntax highlighting
- **Lists** - Use consistent bullet point formatting
- **Links** - Use descriptive link text
- **Images** - Include alt text for accessibility

## Version Information

- **Documentation Version**: 1.0.0
- **Last Updated**: August 2025
- **Project Version**: 1.0.0
- **Compatibility**: All current system versions

## License

This documentation is licensed under the MIT License. See the [LICENSE](../../LICENSE) file for details.

## Acknowledgments

Thanks to all contributors who have helped create and improve this documentation:

- **Project Team** - Core development and testing
- **Community Contributors** - Bug reports and improvements
- **Open Source Projects** - Tools and libraries used
- **Documentation Tools** - Markdown, VuePress, and more

---

**Need help?** Check the [User Guide](user-guide/README.md) or [create an issue](../../issues/new) for support.
