#!/usr/bin/env python3
"""
TPMS Simulator for Mock Sensor Hub
Simulates tire pressure and temperature sensors with configurable mappings
"""

import random
import logging
from datetime import datetime
from typing import Dict, List, Optional, Any

logger = logging.getLogger(__name__)

class TPMSSimulator:
	def __init__(self, config: Dict):
		self.config = config
		self.enabled = config.get('enabled', True)
		self.sensors = config.get('sensors', [])
		self.sensor_data = {}
		self.initialize_sensors()

	def initialize_sensors(self):
		"""Initialize default TPMS sensors if none configured"""
		if not self.sensors:
			# Default sensor configuration
			self.sensors = [
				{
					'id': 'FL',  # Front Left
					'name': 'Front Left Tire',
					'position': 'FL',
					'target_pressure': 32.0,  # PSI
					'pressure_range': (28.0, 36.0),
					'temp_range': (40.0, 120.0),  # Fahrenheit
					'battery_level': 100
				},
				{
					'id': 'FR',  # Front Right
					'name': 'Front Right Tire',
					'position': 'FR',
					'target_pressure': 32.0,
					'pressure_range': (28.0, 36.0),
					'temp_range': (40.0, 120.0),
					'battery_level': 100
				},
				{
					'id': 'RL',  # Rear Left
					'name': 'Rear Left Tire',
					'position': 'RL',
					'target_pressure': 30.0,
					'pressure_range': (26.0, 34.0),
					'temp_range': (40.0, 120.0),
					'battery_level': 100
				},
				{
					'id': 'RR',  # Rear Right
					'name': 'Rear Right Tire',
					'position': 'RR',
					'target_pressure': 30.0,
					'pressure_range': (26.0, 34.0),
					'temp_range': (40.0, 120.0),
					'battery_level': 100
				}
			]

			# Initialize sensor data
			for sensor in self.sensors:
				self.sensor_data[sensor['id']] = {
					'pressure': sensor['target_pressure'],
					'temperature': 70.0,  # Start at room temperature
					'battery': sensor['battery_level'],
					'last_update': datetime.now(),
					'status': 'normal'
				}

	def update_sensor(self, sensor_id: str, pressure: float = None,
			temperature: float = None, battery: float = None):
		"""Update a specific sensor's values"""
		if sensor_id not in self.sensor_data:
			logger.warning(f"Unknown TPMS sensor ID: {sensor_id}")
			return

		if pressure is not None:
			self.sensor_data[sensor_id]['pressure'] = pressure
		if temperature is not None:
			self.sensor_data[sensor_id]['temperature'] = temperature
		if battery is not None:
			self.sensor_data[sensor_id]['battery'] = battery

		self.sensor_data[sensor_id]['last_update'] = datetime.now()
		self._update_sensor_status(sensor_id)

	def _update_sensor_status(self, sensor_id: str):
		"""Update sensor status based on current values"""
		sensor = self.sensor_data[sensor_id]
		config = next((s for s in self.sensors if s['id'] == sensor_id), None)

		if not config:
			return

		pressure = sensor['pressure']
		temp = sensor['temperature']
		battery = sensor['battery']

		# Check pressure status
		if pressure < config['pressure_range'][0]:
			sensor['status'] = 'low_pressure'
		elif pressure > config['pressure_range'][1]:
			sensor['status'] = 'high_pressure'
		elif pressure < config['target_pressure'] * 0.9:
			sensor['status'] = 'warning'
		else:
			sensor['status'] = 'normal'

		# Check temperature status
		if temp > config['temp_range'][1]:
			sensor['status'] = 'overheating'

		# Check battery status
		if battery < 20:
			sensor['status'] = 'low_battery'

	def simulate_reading(self, sensor_id: str) -> Dict:
		"""Simulate a realistic sensor reading with some variation"""
		if sensor_id not in self.sensor_data:
			return None

		config = next((s for s in self.sensors if s['id'] == sensor_id), None)
		if not config:
			return None

		current_data = self.sensor_data[sensor_id]

		# Add realistic variations
		pressure_variation = random.uniform(-0.5, 0.5)
		temp_variation = random.uniform(-2.0, 2.0)
		battery_drain = random.uniform(0.0, 0.1)  # Very slow battery drain

		# Update values with realistic constraints
		new_pressure = max(config['pressure_range'][0],
			min(config['pressure_range'][1],
			current_data['pressure'] + pressure_variation))

		new_temp = max(config['temp_range'][0],
			min(config['temp_range'][1],
			current_data['temperature'] + temp_variation))

		new_battery = max(0, current_data['battery'] - battery_drain)

		# Update sensor data
		self.update_sensor(sensor_id, new_pressure, new_temp, new_battery)

		return {
			'sensor_id': sensor_id,
			'name': config['name'],
			'position': config['position'],
			'pressure': round(new_pressure, 1),
			'temperature': round(new_temp, 1),
			'battery': round(new_battery, 1),
			'status': self.sensor_data[sensor_id]['status'],
			'target_pressure': config['target_pressure'],
			'last_update': self.sensor_data[sensor_id]['last_update'].isoformat()
		}

	def simulate_all_readings(self) -> List[Dict]:
		"""Simulate readings for all sensors"""
		if not self.enabled:
			return []

		readings = []
		for sensor in self.sensors:
			reading = self.simulate_reading(sensor['id'])
			if reading:
				readings.append(reading)

		return readings

	def get_sensor_config(self) -> List[Dict]:
		"""Get current sensor configuration"""
		return self.sensors.copy()

	def update_sensor_config(self, sensor_id: str, new_config: Dict):
		"""Update sensor configuration"""
		for i, sensor in enumerate(self.sensors):
			if sensor['id'] == sensor_id:
				self.sensors[i].update(new_config)
				logger.info(f"Updated TPMS sensor config for {sensor_id}")
				return

		logger.warning(f"TPMS sensor {sensor_id} not found for config update")

	def add_sensor(self, sensor_config: Dict):
		"""Add a new TPMS sensor"""
		if 'id' not in sensor_config:
			logger.error("TPMS sensor config must include 'id' field")
			return

		# Check if sensor already exists
		if any(s['id'] == sensor_config['id'] for s in self.sensors):
			logger.warning(f"TPMS sensor {sensor_config['id']} already exists")
			return

		# Set defaults for missing fields
		defaults = {
			'target_pressure': 32.0,
			'pressure_range': (28.0, 36.0),
			'temp_range': (40.0, 120.0),
			'battery_level': 100
		}

		for key, default_value in defaults.items():
			if key not in sensor_config:
				sensor_config[key] = default_value

		self.sensors.append(sensor_config)

		# Initialize sensor data
		self.sensor_data[sensor_config['id']] = {
			'pressure': sensor_config['target_pressure'],
			'temperature': 70.0,
			'battery': sensor_config['battery_level'],
			'last_update': datetime.now(),
			'status': 'normal'
		}

		logger.info(f"Added new TPMS sensor: {sensor_config['id']}")

	def remove_sensor(self, sensor_id: str):
		"""Remove a TPMS sensor"""
		# Remove from sensors list
		self.sensors = [s for s in self.sensors if s['id'] != sensor_id]

		# Remove from sensor data
		if sensor_id in self.sensor_data:
			del self.sensor_data[sensor_id]

		logger.info(f"Removed TPMS sensor: {sensor_id}")

	def get_sensor_status_summary(self) -> Dict:
		"""Get summary of all sensor statuses"""
		if not self.enabled:
			return {'enabled': False, 'sensors': []}

		summary = {
			'enabled': True,
			'total_sensors': len(self.sensors),
			'sensors': []
		}

		for sensor in self.sensors:
			sensor_id = sensor['id']
			if sensor_id in self.sensor_data:
				data = self.sensor_data[sensor_id]
				summary['sensors'].append({
					'id': sensor_id,
					'name': sensor['name'],
					'position': sensor['position'],
					'status': data['status'],
					'pressure': data['pressure'],
					'temperature': data['temperature'],
					'battery': data['battery']
				})

		return summary

	async def update(self, current_time: datetime = None):
		"""Update all TPMS sensors (called by main simulation loop)"""
		if not self.enabled:
			return

		# Simulate readings for all sensors
		readings = self.simulate_all_readings()

		# Update sensor data with new readings
		for reading in readings:
			sensor_id = reading['sensor_id']
			if sensor_id in self.sensor_data:
				self.sensor_data[sensor_id].update({
					'pressure': reading['pressure'],
					'temperature': reading['temperature'],
					'battery': reading['battery'],
					'status': reading['status'],
					'last_update': datetime.now()
				})

	def get_data(self) -> List[Dict]:
		"""Get current TPMS sensor data for API responses"""
		if not self.enabled:
			return []

		data = []
		for sensor_id, sensor_data in self.sensor_data.items():
			config = next((s for s in self.sensors if s['id'] == sensor_id), None)
			if config:
				data.append({
					'sensor_id': sensor_id,
					'name': config['name'],
					'position': config['position'],
					'pressure': sensor_data['pressure'],
					'temperature': sensor_data['temperature'],
					'battery': sensor_data['battery'],
					'status': sensor_data['status'],
					'target_pressure': config['target_pressure'],
					'last_update': sensor_data['last_update'].isoformat()
				})
		return data
