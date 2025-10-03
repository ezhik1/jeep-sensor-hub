#!/usr/bin/env python3
"""
Engine Simulator for Mock Sensor Hub
Simulates engine-related sensors including oil pressure, coolant temperature, etc.
"""

import random
import math
import logging
from datetime import datetime, timedelta
from typing import Dict, List, Optional, Any

logger = logging.getLogger(__name__)

class EngineSimulator:
	def __init__(self, config: Dict):
		self.config = config
		self.enabled = config.get('enabled', True)
		self.update_interval = config.get('update_interval', 1.0)  # seconds

		# Engine configuration
		self.engine_type = config.get('engine_type', 'gasoline')
		self.cylinders = config.get('cylinders', 6)
		self.displacement = config.get('displacement', 4.0)  # liters
		self.max_rpm = config.get('max_rpm', 6000)

		# Engine state
		self.engine_running = False
		self.rpm = 0
		self.throttle_position = 0.0  # 0.0 = closed, 1.0 = wide open
		self.engine_load = 0.0  # 0.0 = no load, 1.0 = full load
		self.engine_temp = 70.0  # °F, ambient temperature when cold

		# Sensor configurations
		self.sensors = config.get('sensors', {})
		self.sensor_data = {}

		# Initialize default sensors if none configured
		self.initialize_default_sensors()

		# Engine cycle tracking
		self.engine_start_time = None
		self.total_runtime = 0.0  # seconds
		self.engine_cycles = 0

		# Performance tracking
		self.max_rpm_reached = 0
		self.max_temp_reached = 70.0
		self.max_oil_pressure = 0.0

	def initialize_default_sensors(self):
		"""Initialize default engine sensors if none configured"""
		if not self.sensors:
			self.sensors = {
				'oil_pressure': {
					'type': 'pressure',
					'unit': 'PSI',
					'min_value': 0.0,
					'max_value': 80.0,
					'normal_range': (15.0, 60.0),
					'warning_range': (10.0, 70.0),
					'critical_range': (5.0, 80.0),
					'response_time': 0.5,  # seconds to reach target
					'description': 'Engine Oil Pressure'
				},
				'coolant_temp': {
					'type': 'temperature',
					'unit': '°F',
					'min_value': 70.0,
					'max_value': 250.0,
					'normal_range': (160.0, 220.0),
					'warning_range': (140.0, 230.0),
					'critical_range': (130.0, 240.0),
					'response_time': 2.0,
					'description': 'Engine Coolant Temperature'
				},
				'oil_temp': {
					'type': 'temperature',
					'unit': '°F',
					'min_value': 70.0,
					'max_value': 280.0,
					'normal_range': (180.0, 240.0),
					'warning_range': (160.0, 260.0),
					'critical_range': (150.0, 270.0),
					'response_time': 3.0,
					'description': 'Engine Oil Temperature'
				},
				'vacuum_pressure': {
					'type': 'pressure',
					'unit': 'inHg',
					'min_value': 0.0,
					'max_value': 30.0,
					'normal_range': (15.0, 25.0),
					'warning_range': (10.0, 28.0),
					'critical_range': (5.0, 30.0),
					'response_time': 0.2,
					'description': 'Intake Manifold Vacuum'
				},
				'fuel_pressure': {
					'type': 'pressure',
					'unit': 'PSI',
					'min_value': 0.0,
					'max_value': 100.0,
					'normal_range': (45.0, 65.0),
					'warning_range': (35.0, 75.0),
					'critical_range': (25.0, 85.0),
					'response_time': 0.1,
					'description': 'Fuel Rail Pressure'
				}
			}

		# Initialize sensor data
		for sensor_id, sensor_config in self.sensors.items():
			self.sensor_data[sensor_id] = {
				'current_value': sensor_config['min_value'],
				'target_value': sensor_config['min_value'],
				'last_update': datetime.now(),
				'status': 'normal'
			}

	def start_engine(self):
		"""Start the engine simulation"""
		if not self.engine_running:
			self.engine_running = True
			self.engine_start_time = datetime.now()
			self.rpm = 800  # Idle RPM
			self.engine_cycles += 1
			logger.info("Engine started")

	def stop_engine(self):
		"""Stop the engine simulation"""
		if self.engine_running:
			self.engine_running = False
			if self.engine_start_time:
				runtime = (datetime.now() - self.engine_start_time).total_seconds()
				self.total_runtime += runtime
			self.rpm = 0
			self.throttle_position = 0.0
			self.engine_load = 0.0
			logger.info("Engine stopped")

	def set_throttle(self, position: float):
		"""Set throttle position (0.0 to 1.0)"""
		self.throttle_position = max(0.0, min(1.0, position))

		if self.engine_running:
			# Calculate RPM based on throttle position
			if self.throttle_position < 0.1:
				self.rpm = 800  # Idle
			else:
				# RPM increases with throttle, with some randomness
				base_rpm = 800 + (self.throttle_position * (self.max_rpm - 800))
				self.rpm = min(self.max_rpm, base_rpm + random.uniform(-100, 100))

			# Update engine load
			self.engine_load = self.throttle_position * 0.8 + random.uniform(0, 0.2)
			self.engine_load = min(1.0, self.engine_load)

	def simulate_sensor_reading(self, sensor_id: str) -> Optional[Dict]:
		"""Simulate a reading for a specific engine sensor"""
		if sensor_id not in self.sensors or sensor_id not in self.sensor_data:
			return None

		sensor_config = self.sensors[sensor_id]
		sensor_data = self.sensor_data[sensor_id]
		sensor_type = sensor_config['type']

		# Calculate target value based on engine state
		target_value = self._calculate_sensor_target(sensor_id, sensor_config)

		# Gradually move current value toward target
		response_time = sensor_config['response_time']
		time_factor = self.update_interval / response_time

		current_value = sensor_data['current_value']
		value_diff = target_value - current_value

		if abs(value_diff) > 0.1:
			new_value = current_value + (value_diff * time_factor)
		else:
			# Add small random variation around target
			variation = random.uniform(-1.0, 1.0)
			new_value = target_value + variation

		# Apply sensor constraints
		new_value = max(sensor_config['min_value'],
						min(sensor_config['max_value'], new_value))

		# Update sensor data
		sensor_data['current_value'] = new_value
		sensor_data['target_value'] = target_value
		sensor_data['last_update'] = datetime.now()
		sensor_data['status'] = self._determine_sensor_status(sensor_id, new_value, sensor_config)

		return {
			'sensor_id': sensor_id,
			'type': sensor_type,
			'value': round(new_value, 2),
			'unit': sensor_config['unit'],
			'status': sensor_data['status'],
			'description': sensor_config['description'],
			'timestamp': sensor_data['last_update'].isoformat()
		}

	def _calculate_sensor_target(self, sensor_id: str, sensor_config: Dict) -> float:
		"""Calculate target value for a sensor based on engine state"""
		if not self.engine_running:
			return sensor_config['min_value']

		if sensor_id == 'oil_pressure':
			# Oil pressure increases with RPM
			rpm_factor = self.rpm / self.max_rpm
			base_pressure = 15.0  # PSI at idle
			max_pressure = 60.0   # PSI at max RPM
			return base_pressure + (rpm_factor * (max_pressure - base_pressure))

		elif sensor_id == 'coolant_temp':
			# Coolant temperature increases with engine load and runtime
			if self.engine_start_time:
				runtime = (datetime.now() - self.engine_start_time).total_seconds()
				warmup_factor = min(1.0, runtime / 300.0)  # 5 minutes to warm up
			else:
				warmup_factor = 0.0

			base_temp = 70.0  # Ambient
			operating_temp = 195.0  # Normal operating temperature
			load_factor = self.engine_load * 20.0  # Additional heat from load

			return base_temp + (warmup_factor * (operating_temp - base_temp)) + load_factor

		elif sensor_id == 'oil_temp':
			# Oil temperature follows coolant but with lag
			coolant_temp = self.sensor_data.get('coolant_temp', {}).get('current_value', 70.0)
			oil_temp = coolant_temp + random.uniform(-10.0, 5.0)
			return max(70.0, oil_temp)

		elif sensor_id == 'vacuum_pressure':
			# Vacuum decreases with throttle opening
			base_vacuum = 25.0  # inHg at idle
			throttle_factor = self.throttle_position
			return base_vacuum - (throttle_factor * 15.0)

		elif sensor_id == 'fuel_pressure':
			# Fuel pressure is relatively constant with small variations
			base_pressure = 55.0  # PSI
			variation = random.uniform(-5.0, 5.0)
			return base_pressure + variation

		return sensor_config['min_value']

	def _determine_sensor_status(self, sensor_id: str, value: float, sensor_config: Dict) -> str:
		"""Determine sensor status based on current value"""
		normal_range = sensor_config['normal_range']
		warning_range = sensor_config['warning_range']
		critical_range = sensor_config['critical_range']

		if value < critical_range[0] or value > critical_range[1]:
			return 'critical'
		elif value < warning_range[0] or value > warning_range[1]:
			return 'warning'
		elif value < normal_range[0] or value > normal_range[1]:
			return 'elevated'
		else:
			return 'normal'

	def simulate_all_readings(self) -> List[Dict]:
		"""Simulate readings for all engine sensors"""
		if not self.enabled:
			return []

		readings = []
		for sensor_id in self.sensors:
			reading = self.simulate_sensor_reading(sensor_id)
			if reading:
				readings.append(reading)

		return readings

	def get_engine_status(self) -> Dict:
		"""Get current engine status and performance data"""
		if not self.engine_running:
			return {
				'status': 'stopped',
				'rpm': 0,
				'throttle_position': 0.0,
				'engine_load': 0.0,
				'engine_temp': self.engine_temp,
				'runtime': self.total_runtime
			}

		# Update performance tracking
		if self.rpm > self.max_rpm_reached:
			self.max_rpm_reached = self.rpm

		# Get current temperatures for tracking
		coolant_temp = self.sensor_data.get('coolant_temp', {}).get('current_value', 70.0)
		if coolant_temp > self.max_temp_reached:
			self.max_temp_reached = coolant_temp

		oil_pressure = self.sensor_data.get('oil_pressure', {}).get('current_value', 0.0)
		if oil_pressure > self.max_oil_pressure:
			self.max_oil_pressure = oil_pressure

		return {
			'status': 'running',
			'rpm': self.rpm,
			'throttle_position': self.throttle_position,
			'engine_load': self.engine_load,
			'engine_temp': coolant_temp,
			'runtime': self.total_runtime + (datetime.now() - self.engine_start_time).total_seconds(),
			'performance': {
				'max_rpm_reached': self.max_rpm_reached,
				'max_temp_reached': self.max_temp_reached,
				'max_oil_pressure': self.max_oil_pressure,
				'engine_cycles': self.engine_cycles
			}
		}

	def get_sensor_config(self) -> Dict:
		"""Get current sensor configuration"""
		return self.sensors.copy()

	def get_sensor_status_summary(self) -> Dict:
		"""Get summary of all sensor statuses"""
		if not self.enabled:
			return {'enabled': False, 'sensors': []}

		summary = {
			'enabled': True,
			'total_sensors': len(self.sensors),
			'engine_status': self.get_engine_status(),
			'sensors': []
		}

		for sensor_id, sensor_data in self.sensor_data.items():
			sensor_config = self.sensors[sensor_id]
			summary['sensors'].append({
				'id': sensor_id,
				'type': sensor_config['type'],
				'current_value': sensor_data['current_value'],
				'unit': sensor_config['unit'],
				'status': sensor_data['status'],
				'description': sensor_config['description'],
				'last_update': sensor_data['last_update'].isoformat()
			})

		return summary

	def add_sensor(self, sensor_id: str, sensor_config: Dict):
		"""Add a new engine sensor"""
		if sensor_id in self.sensors:
			logger.warning(f"Engine sensor {sensor_id} already exists")
			return

		# Validate required fields
		required_fields = ['type', 'unit', 'min_value', 'max_value', 'normal_range', 'description']
		for field in required_fields:
			if field not in sensor_config:
				logger.error(f"Missing required field '{field}' for sensor {sensor_id}")
				return

		self.sensors[sensor_id] = sensor_config
		self.sensor_data[sensor_id] = {
			'current_value': sensor_config['min_value'],
			'target_value': sensor_config['min_value'],
			'last_update': datetime.now(),
			'status': 'normal'
		}

		logger.info(f"Added new engine sensor: {sensor_id}")

	def remove_sensor(self, sensor_id: str):
		"""Remove an engine sensor"""
		if sensor_id in self.sensors:
			del self.sensors[sensor_id]
			if sensor_id in self.sensor_data:
				del self.sensor_data[sensor_id]
			logger.info(f"Removed engine sensor: {sensor_id}")
		else:
			logger.warning(f"Sensor {sensor_id} not found")

	async def update(self, current_time: datetime = None):
		"""Update engine simulator (called by main simulation loop)"""
		if not self.enabled:
			return

		# Update engine temperature based on runtime and load
		if self.engine_running:
			# Engine warms up over time
			if self.engine_start_time:
				runtime = (current_time - self.engine_start_time).total_seconds()
				warmup_factor = min(1.0, runtime / 300.0)  # 5 minutes to warm up
				self.engine_temp = 70.0 + (warmup_factor * 125.0)  # 70°F to 195°F
			else:
				self.engine_temp = 70.0

		# Simulate readings for all sensors
		readings = self.simulate_all_readings()

		# Log any warnings
		for reading in readings:
			if reading['status'] in ['warning', 'critical']:
				logger.warning(f"Engine sensor {reading['sensor_id']} in {reading['status']} state: {reading['value']} {reading['unit']}")

	def get_data(self) -> Dict:
		"""Get current engine data for API responses"""
		if not self.enabled:
			return {
				'enabled': False,
				'error': 'Engine simulator disabled'
			}

		return {
			'enabled': True,
			'engine_status': self.get_engine_status(),
			'sensors': self.simulate_all_readings(),
			'configuration': {
				'engine_type': self.engine_type,
				'cylinders': self.cylinders,
				'displacement': self.displacement,
				'max_rpm': self.max_rpm
			}
		}
