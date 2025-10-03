# Development Guide

This directory contains comprehensive development documentation for the Jeep Sensor Hub system, including development setup, coding standards, testing procedures, and contribution guidelines.

## Overview

The Jeep Sensor Hub is an open-source project that welcomes contributions from developers, hardware enthusiasts, and automotive professionals. This guide provides everything needed to set up a development environment, understand the codebase, and contribute to the project.

## Development Environment Setup

### Prerequisites
- **Python**: 3.9+ (3.13 recommended)
- **Node.js**: 18+ LTS
- **Git**: Latest version
- **Text Editor**: VS Code, PyCharm, or Vim
- **Hardware**: Raspberry Pi Zero, ESP32 boards (optional for development)

### Local Development Setup

#### 1. Clone Repository
```bash
git clone https://github.com/your-org/jeep-sensor-hub.git
cd jeep-sensor-hub
```

#### 2. Python Environment Setup
```bash
# Create virtual environment
python -m venv venv

# Activate virtual environment
# Windows
venv\Scripts\activate
# macOS/Linux
source venv/bin/activate

# Install dependencies
pip install -r mock-sensor-hub/requirements.txt
pip install -r sensor-hub/requirements.txt
```

#### 3. Node.js Environment Setup
```bash
# Install Node.js dependencies
cd web-frontend
npm install

cd ../backend-api
npm install
```

#### 4. Development Tools
```bash
# Install development tools
pip install black flake8 mypy pytest pytest-asyncio
npm install -g eslint prettier
```

### IDE Configuration

#### VS Code Extensions
```json
{
  "recommendations": [
    "ms-python.python",
    "ms-python.black-formatter",
    "ms-python.flake8",
    "ms-python.mypy-type-checker",
    "ms-vscode.vscode-typescript-next",
    "bradlc.vscode-tailwindcss",
    "esbenp.prettier-vscode"
  ]
}
```

#### VS Code Settings
```json
{
  "python.defaultInterpreterPath": "./venv/bin/python",
  "python.formatting.provider": "black",
  "python.linting.enabled": true,
  "python.linting.flake8Enabled": true,
  "editor.formatOnSave": true,
  "editor.codeActionsOnSave": {
    "source.organizeImports": true
  }
}
```

## Project Structure

### Directory Overview
```
jeep-sensor-hub/
├── mock-sensor-hub/          # Mock sensor hub for development
├── sensor-hub/               # Real sensor hub implementation
├── esp32-displays/           # ESP32 display modules
├── web-frontend/             # Vue.js web interface
├── backend-api/              # Node.js/Express backend
├── hardware/                 # Hardware documentation
├── docs/                     # Project documentation
└── README.md                 # Project overview
```

### Key Components

#### Mock Sensor Hub
- **Purpose**: Development and testing without hardware
- **Technology**: Python, FastAPI, SQLite
- **Features**: Sensor simulation, data generation, API endpoints

#### Real Sensor Hub
- **Purpose**: Production hardware implementation
- **Technology**: Python, Raspberry Pi Zero, GPIO
- **Features**: Real sensor reading, hardware control, data logging

#### ESP32 Displays
- **Purpose**: Vehicle display modules
- **Technology**: MicroPython, OLED displays, buttons
- **Features**: Real-time data display, user interaction

#### Web Frontend
- **Purpose**: Web-based monitoring interface
- **Technology**: Vue.js 3, Pinia, Chart.js
- **Features**: Real-time dashboards, data visualization, mobile responsive

#### Backend API
- **Purpose**: Middleware and data management
- **Technology**: Node.js, Express, WebSocket
- **Features**: Data aggregation, authentication, real-time updates

## Coding Standards

### Python Standards

#### Code Style
- **Formatter**: Black (line length: 88)
- **Linter**: Flake8
- **Type Checking**: MyPy
- **Import Sorting**: isort

#### Naming Conventions
```python
# Variables and functions: snake_case
temperature_sensor = "DHT22"
def read_temperature():
    pass

# Classes: PascalCase
class TemperatureManager:
    pass

# Constants: UPPER_SNAKE_CASE
MAX_TEMPERATURE = 120.0
DEFAULT_TIMEOUT = 5000

# Private methods: _leading_underscore
def _internal_helper():
    pass
```

#### Documentation
```python
def calculate_fan_speed(temperature: float, threshold: float) -> int:
    """
    Calculate the required fan speed based on temperature.

    Args:
        temperature: Current temperature in Celsius
        threshold: Temperature threshold for fan activation

    Returns:
        Fan speed level (0=off, 1=low, 2=high)

    Raises:
        ValueError: If temperature is negative
    """
    if temperature < 0:
        raise ValueError("Temperature cannot be negative")

    if temperature > threshold + 10:
        return 2
    elif temperature > threshold:
        return 1
    else:
        return 0
```

### JavaScript/TypeScript Standards

#### Code Style
- **Formatter**: Prettier
- **Linter**: ESLint
- **Framework**: Vue.js 3 Composition API

#### Naming Conventions
```javascript
// Variables and functions: camelCase
const temperatureSensor = 'DHT22';
function readTemperature() {
  // ...
}

// Components: PascalCase
const TemperatureDisplay = defineComponent({
  // ...
});

// Constants: UPPER_SNAKE_CASE
const MAX_TEMPERATURE = 120.0;
const DEFAULT_TIMEOUT = 5000;

// Private methods: _leadingUnderscore
function _internalHelper() {
  // ...
}
```

#### Vue.js Best Practices
```vue
<template>
  <div class="temperature-display">
    <h3>{{ title }}</h3>
    <div class="value">{{ formattedTemperature }}</div>
    <div class="unit">{{ unit }}</div>
  </div>
</template>

<script setup>
import { computed, ref } from 'vue';

// Props
const props = defineProps({
  temperature: {
    type: Number,
    required: true
  },
  unit: {
    type: String,
    default: '°C'
  }
});

// Reactive data
const title = ref('Engine Temperature');

// Computed properties
const formattedTemperature = computed(() => {
  return props.temperature.toFixed(1);
});
</script>

<style scoped>
.temperature-display {
  padding: 1rem;
  border: 1px solid #ccc;
  border-radius: 4px;
}
</style>
```

## Testing

### Python Testing

#### Test Structure
```python
# test_sensor_manager.py
import pytest
from unittest.mock import Mock, patch
from sensor_hub.src.sensors.sensor_manager import SensorManager

class TestSensorManager:
    @pytest.fixture
    def sensor_manager(self):
        return SensorManager()

    def test_initialization(self, sensor_manager):
        assert sensor_manager.sensors == {}
        assert sensor_manager.is_running is False

    @patch('sensor_hub.src.sensors.sensor_manager.GPIO')
    def test_gpio_setup(self, mock_gpio, sensor_manager):
        sensor_manager.setup_gpio()
        mock_gpio.setmode.assert_called_once()

    @pytest.mark.asyncio
    async def test_sensor_reading(self, sensor_manager):
        result = await sensor_manager.read_sensor('temperature')
        assert 'value' in result
        assert 'timestamp' in result
```

#### Running Tests
```bash
# Run all tests
pytest

# Run specific test file
pytest tests/test_sensor_manager.py

# Run with coverage
pytest --cov=sensor_hub --cov-report=html

# Run async tests
pytest --asyncio-mode=auto
```

### JavaScript Testing

#### Test Structure
```javascript
// TemperatureDisplay.test.js
import { mount } from '@vue/test-utils';
import { describe, it, expect } from 'vitest';
import TemperatureDisplay from '@/components/TemperatureDisplay.vue';

describe('TemperatureDisplay', () => {
  it('renders temperature value correctly', () => {
    const wrapper = mount(TemperatureDisplay, {
      props: {
        temperature: 85.5,
        unit: '°C'
      }
    });

    expect(wrapper.text()).toContain('85.5');
    expect(wrapper.text()).toContain('°C');
  });

  it('formats temperature to one decimal place', () => {
    const wrapper = mount(TemperatureDisplay, {
      props: {
        temperature: 85.567,
        unit: '°C'
      }
    });

    expect(wrapper.text()).toContain('85.6');
  });
});
```

#### Running Tests
```bash
# Run all tests
npm test

# Run with coverage
npm run test:coverage

# Run specific test file
npm test TemperatureDisplay.test.js
```

## Development Workflow

### Feature Development

#### 1. Create Feature Branch
```bash
git checkout -b feature/temperature-alerts
```

#### 2. Implement Feature
- Write code following coding standards
- Add comprehensive tests
- Update documentation
- Ensure all tests pass

#### 3. Commit Changes
```bash
git add .
git commit -m "feat: add temperature alert system

- Implement temperature threshold monitoring
- Add configurable alert levels
- Include email and push notifications
- Add comprehensive test coverage"
```

#### 4. Push and Create PR
```bash
git push origin feature/temperature-alerts
# Create Pull Request on GitHub
```

### Bug Fixes

#### 1. Create Bug Fix Branch
```bash
git checkout -b fix/temperature-sensor-crash
```

#### 2. Fix the Issue
- Identify root cause
- Implement fix
- Add regression tests
- Update documentation if needed

#### 3. Commit Fix
```bash
git commit -m "fix: prevent temperature sensor crash on invalid data

- Add input validation for temperature readings
- Handle sensor communication errors gracefully
- Add error logging for debugging
- Include test cases for error conditions"
```

### Code Review Process

#### Pull Request Checklist
- [ ] Code follows project standards
- [ ] All tests pass
- [ ] Documentation is updated
- [ ] No breaking changes (or documented)
- [ ] Performance impact considered
- [ ] Security implications reviewed

#### Review Guidelines
- **Be constructive**: Focus on code, not person
- **Ask questions**: Clarify unclear implementations
- **Suggest alternatives**: Provide specific suggestions
- **Test thoroughly**: Verify fixes work as expected

## Debugging and Troubleshooting

### Python Debugging

#### Logging
```python
import logging

# Configure logging
logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)

logger = logging.getLogger(__name__)

# Use in code
logger.debug("Reading temperature sensor")
logger.info("Temperature: 85.2°C")
logger.warning("Temperature approaching threshold")
logger.error("Sensor communication failed")
```

#### Debug Mode
```python
# Enable debug mode
import os
os.environ['DEBUG'] = '1'

# Use debugger
import pdb; pdb.set_trace()

# Or use breakpoint() (Python 3.7+)
breakpoint()
```

### JavaScript Debugging

#### Console Logging
```javascript
// Debug logging
console.log('Temperature reading:', temperature);
console.warn('Temperature high:', temperature);
console.error('Sensor error:', error);

// Object inspection
console.table(sensorData);
console.group('Sensor Readings');
console.log('Temperature:', temp);
console.log('Humidity:', humidity);
console.groupEnd();
```

#### Browser DevTools
- **Console**: View logs and errors
- **Network**: Monitor API calls
- **Elements**: Inspect DOM structure
- **Sources**: Debug JavaScript code

### Hardware Debugging

#### GPIO Testing
```bash
# Test GPIO access
sudo python3 -c "import RPi.GPIO as GPIO; GPIO.setmode(GPIO.BCM); print('GPIO OK')"

# Test specific pins
sudo python3 -c "import RPi.GPIO as GPIO; GPIO.setup(18, GPIO.OUT); GPIO.output(18, GPIO.HIGH); print('Pin 18 HIGH')"
```

#### I2C Testing
```bash
# Check I2C devices
sudo i2cdetect -y 1

# Test I2C communication
sudo python3 -c "import smbus; bus = smbus.SMBus(1); print('I2C OK')"
```

## Performance Optimization

### Python Optimization

#### Profiling
```python
import cProfile
import pstats

# Profile function
def profile_function():
    profiler = cProfile.Profile()
    profiler.enable()

    # Your code here
    result = expensive_operation()

    profiler.disable()
    stats = pstats.Stats(profiler)
    stats.sort_stats('cumulative')
    stats.print_stats()

    return result
```

#### Memory Optimization
```python
# Use generators for large datasets
def sensor_data_generator():
    for i in range(1000000):
        yield generate_sensor_reading(i)

# Use context managers for resources
with open('data.log', 'r') as f:
    data = f.read()
```

### JavaScript Optimization

#### Performance Monitoring
```javascript
// Performance timing
const start = performance.now();
expensiveOperation();
const end = performance.now();
console.log(`Operation took ${end - start} milliseconds`);

// Memory usage
console.log('Memory usage:', performance.memory);
```

#### Code Splitting
```javascript
// Lazy load components
const TemperatureChart = defineAsyncComponent(() =>
  import('./TemperatureChart.vue')
);

// Dynamic imports
const loadModule = async () => {
  const module = await import('./heavyModule.js');
  return module.default;
};
```

## Security Considerations

### Input Validation
```python
# Validate sensor inputs
def validate_temperature(temp: float) -> float:
    if not isinstance(temp, (int, float)):
        raise ValueError("Temperature must be numeric")

    if temp < -50 or temp > 200:
        raise ValueError("Temperature out of valid range")

    return float(temp)

# Sanitize user inputs
import html
def sanitize_input(user_input: str) -> str:
    return html.escape(user_input.strip())
```

### Authentication
```javascript
// JWT token validation
const validateToken = (token) => {
  try {
    const decoded = jwt.verify(token, process.env.JWT_SECRET);
    return decoded;
  } catch (error) {
    throw new Error('Invalid token');
  }
};

// Rate limiting
const rateLimiter = rateLimit({
  windowMs: 15 * 60 * 1000, // 15 minutes
  max: 100 // limit each IP to 100 requests per windowMs
});
```

## Documentation

### Code Documentation
- **Docstrings**: Use Google or NumPy style
- **Type Hints**: Include for all functions
- **Examples**: Provide usage examples
- **API Docs**: Auto-generate from code

### Project Documentation
- **README**: Project overview and quick start
- **Architecture**: System design and components
- **API Reference**: Complete endpoint documentation
- **User Guide**: End-user instructions

## Deployment

### Development Deployment
```bash
# Start mock sensor hub
cd mock-sensor-hub
uvicorn main:app --reload --host 0.0.0.0 --port 8000

# Start web frontend
cd web-frontend
npm run dev

# Start backend API
cd backend-api
npm run dev
```

### Production Deployment
```bash
# Build frontend
cd web-frontend
npm run build

# Start production services
sudo systemctl start jeep-sensor-hub
sudo systemctl start jeep-backend-api
```

## Contributing

### Getting Started
1. **Fork the repository**
2. **Create a feature branch**
3. **Make your changes**
4. **Add tests**
5. **Update documentation**
6. **Submit a pull request**

### Contribution Areas
- **Bug fixes**: Identify and fix issues
- **New features**: Implement requested functionality
- **Documentation**: Improve guides and examples
- **Testing**: Add test coverage
- **Performance**: Optimize code and systems

### Code of Conduct
- **Be respectful**: Treat others with courtesy
- **Be inclusive**: Welcome diverse perspectives
- **Be constructive**: Provide helpful feedback
- **Be collaborative**: Work together for better code

## Support and Resources

### Development Resources
- **GitHub Issues**: Bug reports and feature requests
- **Discord Server**: Real-time development discussions
- **Documentation**: Comprehensive guides and references
- **Examples**: Code examples and tutorials

### Learning Resources
- **Python**: Official Python documentation
- **Vue.js**: Vue.js 3 documentation
- **Raspberry Pi**: GPIO and hardware guides
- **MicroPython**: ESP32 development guides

### Community
- **Contributors**: Active developers and maintainers
- **Users**: End users and enthusiasts
- **Partners**: Hardware and software partners
- **Mentors**: Experienced developers available for help

## License

This development guide is licensed under the MIT License. See the LICENSE file for details.
