#!/usr/bin/env python3
"""
Power Simulator for Mock Sensor Hub
Simulates power monitoring sensors including voltage, current, and battery levels
"""

import random
import math
import logging
from datetime import datetime, timedelta
from typing import Dict, List, Optional, Any

logger = logging.getLogger(__name__)

class PowerSimulator:
	def __init__(self, config: Dict):
		self.config = config
		self.enabled = config.get('enabled', True)
		self.update_interval = config.get('update_interval', 1.0)  # seconds

		# Power system configuration
		self.systems = config.get('systems', {})
		self.system_data = {}

		# Initialize default power systems if none configured
		self.initialize_default_systems()

		# Power state tracking
		self.engine_running = False
		self.charging_state = 'discharging'  # charging, discharging, float
		self.solar_available = True
		self.alternator_active = False

		# Time-based factors
		self.time_of_day = datetime.now().hour
		self.day_of_year = datetime.now().timetuple().tm_yday

	def initialize_default_systems(self):
		"""Initialize default power systems if none configured"""
		if not self.systems:
			self.systems = {
				'starter_battery': {
					'type': 'battery',
					'voltage_nominal': 12.6,  # V
					'voltage_range': (10.5, 14.4),
					'capacity_ah': 65.0,
					'current_charge_ah': 52.0,  # 80% charged
					'internal_resistance': 0.02,  # ohms
					'temperature': 70.0,  # °F
					'age_factor': 1.0,  # 1.0 = new, 0.8 = old
					'description': 'Starter Battery'
				},
				'house_battery': {
					'type': 'battery',
					'voltage_nominal': 12.6,
					'voltage_range': (10.5, 14.4),
					'capacity_ah': 100.0,
					'current_charge_ah': 75.0,  # 75% charged
					'internal_resistance': 0.015,
					'temperature': 68.0,
					'age_factor': 0.9,
					'description': 'House Battery'
				},
				'refrigerator': {
					'type': 'load',
					'voltage': 12.0,
					'current_draw': 2.5,  # A
					'power_factor': 0.95,
					'duty_cycle': 0.3,  # 30% on time
					'compressor_running': False,
					'description': 'Refrigerator'
				},
				'solar_input': {
					'type': 'source',
					'voltage': 18.0,  # Solar panel voltage
					'current_max': 8.0,  # A
					'efficiency': 0.85,
					'shading_factor': 0.0,  # 0 = no shading, 1 = fully shaded
					'description': 'Solar Panel Input'
				},
				'alternator': {
					'type': 'source',
					'voltage': 14.2,
					'current_max': 60.0,  # A
					'efficiency': 0.75,
					'active': False,
					'description': 'Alternator'
				}
			}

		# Initialize system data
		for system_id, system_config in self.systems.items():
			self.system_data[system_id] = {
				'current_voltage': system_config.get('voltage_nominal', 12.0),
				'current_current': 0.0,
				'power': 0.0,
				'last_update': datetime.now(),
				'status': 'normal'
			}

	def update_power_state(self, engine_running: bool = None, solar_available: bool = None):
		"""Update power system state"""
		if engine_running is not None:
			self.engine_running = engine_running
			self.alternator_active = engine_running

		if solar_available is not None:
			self.solar_available = solar_available

		# Update charging state based on power sources
		self._update_charging_state()

	def _update_charging_state(self):
		"""Update charging state based on available power sources"""
		if self.alternator_active and self.solar_available:
			self.charging_state = 'charging'
		elif self.alternator_active:
			self.charging_state = 'charging'
		elif self.solar_available:
			self.charging_state = 'charging'
		else:
			self.charging_state = 'discharging'

	def simulate_system_reading(self, system_id: str) -> Optional[Dict]:
		"""Simulate a reading for a specific power system"""
		if system_id not in self.systems or system_id not in self.system_data:
			return None

		system_config = self.systems[system_id]
		system_data = self.system_data[system_id]
		system_type = system_config['type']

		if system_type == 'battery':
			reading = self._simulate_battery_reading(system_id, system_config, system_data)
		elif system_type == 'load':
			reading = self._simulate_load_reading(system_id, system_config, system_data)
		elif system_type == 'source':
			reading = self._simulate_source_reading(system_id, system_config, system_data)
		else:
			reading = None

		if reading:
			# Update system data
			system_data.update({
				'current_voltage': reading['voltage'],
				'current_current': reading['current'],
				'power': reading['power'],
				'last_update': datetime.now(),
				'status': reading['status']
			})

		return reading

	def _simulate_battery_reading(self, system_id: str, config: Dict, data: Dict) -> Dict:
		"""Simulate battery system reading"""
		# Calculate current charge level
		charge_percentage = config['current_charge_ah'] / config['capacity_ah']

		# Base voltage based on charge level
		base_voltage = 10.5 + (charge_percentage * 3.9)  # 10.5V to 14.4V range

		# Temperature compensation
		temp_factor = 1.0 + ((config['temperature'] - 77.0) * 0.0003)  # 77°F = 25°C reference

		# Age factor
		age_voltage = base_voltage * config['age_factor']

		# Add realistic variations
		voltage_variation = random.uniform(-0.1, 0.1)
		current_voltage = age_voltage * temp_factor + voltage_variation

		# Calculate current (positive = charging, negative = discharging)
		if self.charging_state == 'charging':
			# Charging current depends on available sources
			if self.alternator_active:
				charge_current = random.uniform(5.0, 15.0)  # 5-15A charging
			elif self.solar_available:
				charge_current = random.uniform(1.0, 5.0)   # 1-5A solar charging
			else:
				charge_current = 0.0
		else:
			# Discharging current depends on loads
			charge_current = -random.uniform(0.5, 3.0)  # 0.5-3A discharging

		# Calculate power
		power = current_voltage * charge_current

		# Determine status
		status = self._determine_battery_status(system_id, current_voltage, charge_percentage)

		return {
			'system_id': system_id,
			'type': 'battery',
			'voltage': round(current_voltage, 2),
			'current': round(charge_current, 2),
			'power': round(power, 2),
			'charge_percentage': round(charge_percentage * 100, 1),
			'temperature': config['temperature'],
			'status': status,
			'description': config['description'],
			'timestamp': datetime.now().isoformat()
		}

	def _simulate_load_reading(self, system_id: str, config: Dict, data: Dict) -> Dict:
		"""Simulate load system reading"""
		# Determine if load is active
		if system_id == 'refrigerator':
			# Refrigerator has duty cycle and compressor state
			if random.random() < config['duty_cycle']:
				config['compressor_running'] = True
				current_draw = config['current_draw']
			else:
				config['compressor_running'] = False
				current_draw = config['current_draw'] * 0.1  # Standby current
		else:
			# Other loads are either on or off
			current_draw = config['current_draw'] if random.random() < 0.7 else 0.0

		# Voltage drop due to current draw
		voltage_drop = current_draw * 0.1  # Simple voltage drop calculation
		current_voltage = 12.0 - voltage_drop

		# Calculate power
		power = current_voltage * current_draw * config.get('power_factor', 1.0)

		# Determine status
		status = 'normal' if current_draw > 0 else 'idle'

		return {
			'system_id': system_id,
			'type': 'load',
			'voltage': round(current_voltage, 2),
			'current': round(current_draw, 2),
			'power': round(power, 2),
			'active': current_draw > 0,
			'status': status,
			'description': config['description'],
			'timestamp': datetime.now().isoformat()
		}

	def _simulate_source_reading(self, system_id: str, config: Dict, data: Dict) -> Dict:
		"""Simulate power source reading"""
		if system_id == 'solar_input':
			# Solar output depends on time of day and weather
			reading = self._simulate_solar_reading(config)
		elif system_id == 'alternator':
			# Alternator output depends on engine state
			reading = self._simulate_alternator_reading(config)
		else:
			reading = None

		return reading

	def _simulate_solar_reading(self, config: Dict) -> Dict:
		"""Simulate solar panel output"""
		# Time of day factor (peak at noon)
		noon_hour = 12
		hours_from_noon = abs(self.time_of_day - noon_hour)
		time_factor = max(0, 1.0 - (hours_from_noon / 6))  # 0 at 6 AM/PM, 1 at noon

		# Seasonal factor (peak in summer)
		day_rad = (self.day_of_year - 172) * 2 * math.pi / 365  # 172 = June 21 (summer solstice)
		seasonal_factor = 0.7 + (0.3 * math.cos(day_rad))  # 0.7 to 1.0 range

		# Calculate available current
		max_current = config['current_max'] * time_factor * seasonal_factor * (1 - config['shading_factor'])
		current_output = max(0, max_current + random.uniform(-0.5, 0.5))

		# Voltage remains relatively constant
		voltage = config['voltage'] + random.uniform(-0.5, 0.5)

		# Calculate power
		power = voltage * current_output * config['efficiency']

		# Determine status
		status = 'active' if current_output > 0.5 else 'inactive'

		return {
			'system_id': 'solar_input',
			'type': 'source',
			'voltage': round(voltage, 2),
			'current': round(current_output, 2),
			'power': round(power, 2),
			'efficiency': config['efficiency'],
			'time_factor': round(time_factor, 2),
			'seasonal_factor': round(seasonal_factor, 2),
			'status': status,
			'description': config['description'],
			'timestamp': datetime.now().isoformat()
		}

	def _simulate_alternator_reading(self, config: Dict) -> Dict:
		"""Simulate alternator output"""
		if not self.alternator_active:
			return {
				'system_id': 'alternator',
				'type': 'source',
				'voltage': 0.0,
				'current': 0.0,
				'power': 0.0,
				'efficiency': config['efficiency'],
				'active': False,
				'status': 'inactive',
				'description': config['description'],
				'timestamp': datetime.now().isoformat()
			}

		# Alternator is active when engine is running
		voltage = config['voltage'] + random.uniform(-0.2, 0.2)

		# Current output depends on battery charge and loads
		if self.charging_state == 'charging':
			# Higher output when charging
			current_output = random.uniform(20.0, config['current_max'])
		else:
			# Lower output when not charging
			current_output = random.uniform(5.0, 15.0)

		# Calculate power
		power = voltage * current_output * config['efficiency']

		return {
			'system_id': 'alternator',
			'type': 'source',
			'voltage': round(voltage, 2),
			'current': round(current_output, 2),
			'power': round(power, 2),
			'efficiency': config['efficiency'],
			'active': True,
			'status': 'active',
			'description': config['description'],
			'timestamp': datetime.now().isoformat()
		}

	def _determine_battery_status(self, system_id: str, voltage: float, charge_percentage: float) -> str:
		"""Determine battery status based on voltage and charge level"""
		if charge_percentage < 0.1:
			return 'critical'
		elif charge_percentage < 0.2:
			return 'low'
		elif voltage < 11.0:
			return 'warning'
		elif voltage > 14.5:
			return 'overcharge'
		else:
			return 'normal'

	def simulate_all_readings(self) -> List[Dict]:
		"""Simulate readings for all power systems"""
		if not self.enabled:
			return []

		readings = []
		for system_id in self.systems:
			reading = self.simulate_system_reading(system_id)
			if reading:
				readings.append(reading)

		return readings

	def get_system_config(self) -> Dict:
		"""Get current system configuration"""
		return self.systems.copy()

	def get_system_status_summary(self) -> Dict:
		"""Get summary of all system statuses"""
		if not self.enabled:
			return {'enabled': False, 'systems': []}

		summary = {
			'enabled': True,
			'total_systems': len(self.systems),
			'power_state': {
				'engine_running': self.engine_running,
				'charging_state': self.charging_state,
				'solar_available': self.solar_available,
				'alternator_active': self.alternator_active
			},
			'systems': []
		}

		for system_id, system_data in self.system_data.items():
			system_config = self.systems[system_id]
			summary['systems'].append({
				'id': system_id,
				'type': system_config['type'],
				'voltage': system_data['current_voltage'],
				'current': system_data['current_current'],
				'power': system_data['power'],
				'status': system_data['status'],
				'description': system_config['description'],
				'last_update': system_data['last_update'].isoformat()
			})

		return summary

	def add_system(self, system_id: str, system_config: Dict):
		"""Add a new power system"""
		if system_id in self.systems:
			logger.warning(f"Power system {system_id} already exists")
			return

		# Validate required fields
		required_fields = ['type', 'description']
		for field in required_fields:
			if field not in system_config:
				logger.error(f"Missing required field '{field}' for system {system_id}")
				return

		self.systems[system_id] = system_config
		self.system_data[system_id] = {
			'current_voltage': system_config.get('voltage_nominal', 12.0),
			'current_current': 0.0,
			'power': 0.0,
			'last_update': datetime.now(),
			'status': 'normal'
		}

		logger.info(f"Added new power system: {system_id}")

	def remove_system(self, system_id: str):
		"""Remove a power system"""
		if system_id in self.systems:
			del self.systems[system_id]
			if system_id in self.system_data:
				del self.system_data[system_id]
			logger.info(f"Removed power system: {system_id}")
		else:
			logger.warning(f"System {system_id} not found")

	def set_battery_charge(self, system_id: str, charge_ah: float):
		"""Set battery charge level"""
		if system_id in self.systems and self.systems[system_id]['type'] == 'battery':
			config = self.systems[system_id]
			config['current_charge_ah'] = max(0, min(config['capacity_ah'], charge_ah))
			logger.info(f"Set {system_id} charge to {charge_ah} Ah")
		else:
			logger.warning(f"System {system_id} is not a battery")

	async def update(self, current_time: datetime = None):
		"""Update power simulator (called by main simulation loop)"""
		if not self.enabled:
			return

		# Update time-based factors
		if current_time:
			self.time_of_day = current_time.hour
			self.day_of_year = current_time.timetuple().tm_yday

		# Simulate readings for all systems
		readings = self.simulate_all_readings()

		# Log any warnings
		for reading in readings:
			if reading['status'] in ['warning', 'low', 'critical']:
				logger.warning(f"Power system {reading['system_id']} in {reading['status']} state: {reading['voltage']}V, {reading['current']}A")

	def get_data(self) -> Dict:
		"""Get current power data for API responses"""
		if not self.enabled:
			return {
				'enabled': False,
				'error': 'Power simulator disabled'
			}

		return {
			'enabled': True,
			'systems': self.simulate_all_readings(),
			'power_state': {
				'engine_running': self.engine_running,
				'charging_state': self.charging_state,
				'solar_available': self.solar_available,
				'alternator_active': self.alternator_active
			},
			'time_factors': {
				'time_of_day': self.time_of_day,
				'day_of_year': self.day_of_year
			}
		}
