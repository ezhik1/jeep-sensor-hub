#!/usr/bin/env python3
"""
Environmental Simulator for Mock Sensor Hub
Simulates temperature, humidity, and other environmental sensors
"""

import random
import math
import logging
from datetime import datetime, timedelta
from typing import Dict, List, Optional, Any

logger = logging.getLogger(__name__)

class EnvironmentalSimulator:
	def __init__(self, config: Dict):
		self.config = config
		self.enabled = config.get('enabled', True)
		self.update_interval = config.get('update_interval', 1.0)  # seconds

		# Sensor configurations
		self.sensors = config.get('sensors', {})
		self.sensor_data = {}

		# Initialize default sensors if none configured
		self.initialize_default_sensors()

		# Environmental factors
		self.time_of_day = datetime.now().hour
		self.weather_conditions = config.get('weather_conditions', 'clear')
		self.engine_heat_factor = 0.0  # Heat from engine
		self.sunlight_factor = 0.0  # Sunlight intensity

	def initialize_default_sensors(self):
		"""Initialize default environmental sensors if none configured"""
		if not self.sensors:
			self.sensors = {
				'outside_temp': {
					'type': 'temperature',
					'unit': '°F',
					'base_value': 70.0,
					'variation_range': (5.0, 15.0),
					'update_rate': 0.1,  # Change per second
					'location': 'outside',
					'description': 'Outside Air Temperature'
				},
				'cabin_temp': {
					'type': 'temperature',
					'unit': '°F',
					'base_value': 72.0,
					'variation_range': (2.0, 8.0),
					'update_rate': 0.05,
					'location': 'cabin',
					'description': 'Cabin Temperature'
				},
				'refrigerator_temp': {
					'type': 'temperature',
					'unit': '°F',
					'base_value': 35.0,
					'variation_range': (1.0, 3.0),
					'update_rate': 0.02,
					'location': 'refrigerator',
					'description': 'Refrigerator Temperature'
				},
				'outside_humidity': {
					'type': 'humidity',
					'unit': '%',
					'base_value': 50.0,
					'variation_range': (10.0, 30.0),
					'update_rate': 0.1,
					'location': 'outside',
					'description': 'Outside Humidity'
				},
				'cabin_humidity': {
					'type': 'humidity',
					'unit': '%',
					'base_value': 45.0,
					'variation_range': (5.0, 15.0),
					'update_rate': 0.05,
					'location': 'cabin',
					'description': 'Cabin Humidity'
				},
				'barometric_pressure': {
					'type': 'pressure',
					'unit': 'hPa',
					'base_value': 1013.25,
					'variation_range': (5.0, 15.0),
					'update_rate': 0.01,
					'location': 'outside',
					'description': 'Barometric Pressure'
				}
			}

		# Initialize sensor data
		for sensor_id, sensor_config in self.sensors.items():
			self.sensor_data[sensor_id] = {
				'current_value': sensor_config['base_value'],
				'target_value': sensor_config['base_value'],
				'last_update': datetime.now(),
				'status': 'normal'
			}

	def update_environmental_factors(self):
		"""Update environmental factors that affect sensor readings"""
		current_time = datetime.now()
		self.time_of_day = current_time.hour

		# Simulate daily temperature cycle
		hour_rad = (self.time_of_day - 6) * math.pi / 12  # Peak at 2 PM (hour 14)
		daily_temp_variation = 15 * math.sin(hour_rad)  # 15°F daily variation

		# Update sunlight factor (peak at noon)
		noon_hour = 12
		hours_from_noon = abs(self.time_of_day - noon_hour)
		self.sunlight_factor = max(0, 1.0 - (hours_from_noon / 6))  # 0 at 6 AM/PM, 1 at noon

		# Update weather conditions (random changes)
		if random.random() < 0.001:  # 0.1% chance per update
			weather_options = ['clear', 'cloudy', 'rainy', 'stormy']
			self.weather_conditions = random.choice(weather_options)
			logger.info(f"Weather changed to: {self.weather_conditions}")

	def simulate_sensor_reading(self, sensor_id: str) -> Optional[Dict]:
		"""Simulate a reading for a specific sensor"""
		if sensor_id not in self.sensors or sensor_id not in self.sensor_data:
			return None

		sensor_config = self.sensors[sensor_id]
		sensor_data = self.sensor_data[sensor_id]

		# Calculate base environmental effects
		base_value = sensor_config['base_value']
		variation_range = sensor_config['variation_range']
		update_rate = sensor_config['update_rate']

		# Apply environmental factors
		environmental_factor = self._calculate_environmental_factor(sensor_id)

		# Calculate new value with realistic variations
		variation = random.uniform(-variation_range[0], variation_range[1])
		time_based_change = update_rate * self.update_interval

		# Gradual drift toward target with environmental influence
		target_diff = sensor_data['target_value'] - sensor_data['current_value']
		drift = target_diff * time_based_change * 0.1  # Slow drift

		new_value = (sensor_data['current_value'] +
					drift +
					variation * 0.1 +
					environmental_factor)

		# Apply sensor-specific constraints
		new_value = self._apply_sensor_constraints(sensor_id, new_value)

		# Update sensor data
		sensor_data['current_value'] = new_value
		sensor_data['last_update'] = datetime.now()
		sensor_data['status'] = self._determine_sensor_status(sensor_id, new_value)

		return {
			'sensor_id': sensor_id,
			'value': round(new_value, 2),
			'unit': sensor_config['unit'],
			'type': sensor_config['type'],
			'location': sensor_config['location'],
			'description': sensor_config['description'],
			'status': sensor_data['status'],
			'timestamp': sensor_data['last_update'].isoformat()
		}

	def _calculate_environmental_factor(self, sensor_id: str) -> float:
		"""Calculate environmental factor for a specific sensor"""
		sensor_config = self.sensors[sensor_id]
		location = sensor_config['location']
		sensor_type = sensor_config['type']

		factor = 0.0

		if sensor_type == 'temperature':
			# Daily temperature cycle
			hour_rad = (self.time_of_day - 6) * math.pi / 12
			daily_variation = 15 * math.sin(hour_rad)

			if location == 'outside':
				factor += daily_variation
				# Weather effects
				if self.weather_conditions == 'rainy':
					factor -= 5.0  # Rain cools things down
				elif self.weather_conditions == 'stormy':
					factor -= 8.0  # Storms are cooler
				elif self.weather_conditions == 'cloudy':
					factor -= 2.0  # Clouds reduce temperature

			elif location == 'cabin':
				# Cabin temperature follows outside but with lag and engine heat
				factor += daily_variation * 0.3  # Reduced outside influence
				factor += self.engine_heat_factor * 0.5  # Engine heat effect
				factor += self.sunlight_factor * 3.0  # Sunlight heats cabin

			elif location == 'refrigerator':
				# Refrigerator maintains stable temperature
				factor += random.uniform(-0.5, 0.5)  # Minimal variation

		elif sensor_type == 'humidity':
			if location == 'outside':
				# Humidity inversely related to temperature
				hour_rad = (self.time_of_day - 6) * math.pi / 12
				daily_temp = 15 * math.sin(hour_rad)
				factor += -daily_temp * 0.5  # Higher temp = lower humidity

				# Weather effects
				if self.weather_conditions == 'rainy':
					factor += 20.0  # Rain increases humidity
				elif self.weather_conditions == 'stormy':
					factor += 25.0  # Storms increase humidity

			elif location == 'cabin':
				# Cabin humidity follows outside but with lag
				factor += random.uniform(-5.0, 5.0)  # Moderate variation

		elif sensor_type == 'pressure':
			if location == 'outside':
				# Barometric pressure changes with weather
				if self.weather_conditions == 'stormy':
					factor -= 20.0  # Low pressure during storms
				elif self.weather_conditions == 'rainy':
					factor -= 10.0  # Slightly lower during rain
				elif self.weather_conditions == 'clear':
					factor += random.uniform(-5.0, 5.0)  # Stable during clear weather

		return factor

	def _apply_sensor_constraints(self, sensor_id: str, value: float) -> float:
		"""Apply physical constraints to sensor values"""
		sensor_config = self.sensors[sensor_id]
		sensor_type = sensor_config['type']

		if sensor_type == 'temperature':
			# Temperature constraints
			if sensor_id == 'refrigerator_temp':
				return max(32.0, min(40.0, value))  # Keep in safe range
			elif sensor_id == 'cabin_temp':
				return max(50.0, min(90.0, value))  # Reasonable cabin range
			else:
				return max(-40.0, min(120.0, value))  # General outdoor range

		elif sensor_type == 'humidity':
			return max(0.0, min(100.0, value))  # Humidity is 0-100%

		elif sensor_type == 'pressure':
			return max(900.0, min(1100.0, value))  # Reasonable pressure range

		return value

	def _determine_sensor_status(self, sensor_id: str, value: float) -> str:
		"""Determine sensor status based on current value"""
		sensor_config = self.sensors[sensor_id]
		sensor_type = sensor_config['type']

		if sensor_type == 'temperature':
			if sensor_id == 'refrigerator_temp':
				if value > 40.0:
					return 'warning'  # Too warm
				elif value < 32.0:
					return 'warning'  # Too cold
				else:
					return 'normal'
			elif sensor_id == 'cabin_temp':
				if value > 85.0:
					return 'warning'  # Too hot
				elif value < 55.0:
					return 'warning'  # Too cold
				else:
					return 'normal'
			else:
				if value > 100.0:
					return 'warning'  # Very hot
				elif value < 0.0:
					return 'warning'  # Very cold
				else:
					return 'normal'

		elif sensor_type == 'humidity':
			if value > 80.0:
				return 'warning'  # Too humid
			elif value < 10.0:
				return 'warning'  # Too dry
			else:
				return 'normal'

		elif sensor_type == 'pressure':
			if value < 950.0:
				return 'warning'  # Low pressure (stormy)
			elif value > 1050.0:
				return 'warning'  # High pressure
			else:
				return 'normal'

		return 'normal'

	def set_engine_heat(self, heat_factor: float):
		"""Set engine heat factor (0.0 = cold, 1.0 = hot)"""
		self.engine_heat_factor = max(0.0, min(1.0, heat_factor))
		logger.debug(f"Engine heat factor set to: {self.engine_heat_factor}")

	def set_weather_conditions(self, conditions: str):
		"""Set weather conditions"""
		valid_conditions = ['clear', 'cloudy', 'rainy', 'stormy']
		if conditions in valid_conditions:
			self.weather_conditions = conditions
			logger.info(f"Weather conditions set to: {conditions}")
		else:
			logger.warning(f"Invalid weather conditions: {conditions}")

	def add_sensor(self, sensor_id: str, sensor_config: Dict):
		"""Add a new environmental sensor"""
		if sensor_id in self.sensors:
			logger.warning(f"Sensor {sensor_id} already exists")
			return

		# Validate required fields
		required_fields = ['type', 'unit', 'base_value', 'variation_range', 'update_rate', 'location', 'description']
		for field in required_fields:
			if field not in sensor_config:
				logger.error(f"Missing required field '{field}' for sensor {sensor_id}")
				return

		self.sensors[sensor_id] = sensor_config
		self.sensor_data[sensor_id] = {
			'current_value': sensor_config['base_value'],
			'target_value': sensor_config['base_value'],
			'last_update': datetime.now(),
			'status': 'normal'
		}

		logger.info(f"Added new environmental sensor: {sensor_id}")

	def remove_sensor(self, sensor_id: str):
		"""Remove an environmental sensor"""
		if sensor_id in self.sensors:
			del self.sensors[sensor_id]
			if sensor_id in self.sensor_data:
				del self.sensor_data[sensor_id]
			logger.info(f"Removed environmental sensor: {sensor_id}")
		else:
			logger.warning(f"Sensor {sensor_id} not found")

	def set_sensor_target(self, sensor_id: str, target_value: float):
		"""Set target value for a sensor"""
		if sensor_id in self.sensor_data:
			self.sensor_data[sensor_id]['target_value'] = target_value
			logger.info(f"Set target for {sensor_id} to {target_value}")
		else:
			logger.warning(f"Sensor {sensor_id} not found")

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
			'weather_conditions': self.weather_conditions,
			'time_of_day': self.time_of_day,
			'engine_heat_factor': self.engine_heat_factor,
			'sunlight_factor': self.sunlight_factor,
			'sensors': []
		}

		for sensor_id, sensor_data in self.sensor_data.items():
			sensor_config = self.sensors[sensor_id]
			summary['sensors'].append({
				'id': sensor_id,
				'type': sensor_config['type'],
				'location': sensor_config['location'],
				'current_value': sensor_data['current_value'],
				'target_value': sensor_data['target_value'],
				'unit': sensor_config['unit'],
				'status': sensor_data['status'],
				'last_update': sensor_data['last_update'].isoformat()
			})

		return summary

	def simulate_all_readings(self) -> List[Dict]:
		"""Simulate readings for all sensors"""
		if not self.enabled:
			return []

		# Update environmental factors
		self.update_environmental_factors()

		readings = []
		for sensor_id in self.sensors:
			reading = self.simulate_sensor_reading(sensor_id)
			if reading:
				readings.append(reading)

		return readings

	async def update(self, current_time: datetime = None):
		"""Update environmental simulator (called by main simulation loop)"""
		if not self.enabled:
			return

		# Simulate readings for all sensors
		readings = self.simulate_all_readings()

		# Log any warnings
		for reading in readings:
			if reading['status'] == 'warning':
				logger.warning(f"Environmental sensor {reading['sensor_id']} in warning state: {reading['value']} {reading['unit']}")

	def get_data(self) -> Dict:
		"""Get current environmental data for API responses"""
		if not self.enabled:
			return {
				'enabled': False,
				'error': 'Environmental simulator disabled'
			}

		return {
			'enabled': True,
			'sensors': self.simulate_all_readings(),
			'environmental_factors': {
				'weather_conditions': self.weather_conditions,
				'time_of_day': self.time_of_day,
				'engine_heat_factor': self.engine_heat_factor,
				'sunlight_factor': self.sunlight_factor
			}
		}
