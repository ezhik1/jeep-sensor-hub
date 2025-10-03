#!/usr/bin/env python3
"""
Cooling Management Simulator
Ports the Arduino cooling controller logic to Python for the sensor hub.
Handles dual-speed fan control, temperature monitoring, and smart buffering.
"""

import asyncio
import random
from datetime import datetime
from typing import Dict, List, Optional
import logging

logger = logging.getLogger(__name__)

class CoolingSimulator:
	"""Simulates the Jeep cooling management system with dual-speed fan control."""

	def __init__(self, config: Dict):
		"""Initialize the cooling simulator with configuration."""
		self.config = config
		self.cooling_config = config.get('cooling', {})

		# Temperature thresholds (in Celsius)
		self.low_speed_trigger_temp = self.cooling_config.get('low_speed_trigger_temp', 93)
		self.high_speed_trigger_temp = self.cooling_config.get('high_speed_trigger_temp', 105)
		self.optimal_temp = self.low_speed_trigger_temp - 2  # Target cooling temperature
		self.min_temp = self.cooling_config.get('min_temp', 60.0)   # 140°F
		self.max_temp = self.cooling_config.get('max_temp', 121.11) # 250°F
		self.overheat_temp = self.cooling_config.get('overheat_temp', 112)  # 239°F
		self.operating_temp = self.cooling_config.get('operating_temp', 90.5)  # 195°F

		# Fan control states
		self.low_speed_fan_running = False
		self.high_speed_fan_running = False
		self.low_speed_override = False
		self.high_speed_override = False
		self.ac_state_active = False

		# Buffering logic (prevents rapid on/off cycling)
		self.should_buffer_cool_high = False
		self.should_buffer_cool_low = False
		self.is_buffer_cooling = False

		# Temperature readings
		self.current_temp = 90.0  # Start at operating temperature
		self.target_temp = 90.0
		self.temp_readings = [90.0] * 3  # Rolling average buffer
		self.read_index = 0

		# Historical data
		self.temp_history = []
		self.fan_state_history = []
		self.max_history_size = 100

		# Simulation parameters
		self.engine_running = False
		self.ambient_temp = 25.0
		self.load_factor = 0.5  # Engine load (0.0 to 1.0)
		self.cooling_efficiency = 0.8

		# Update intervals
		self.temp_update_interval = self.cooling_config.get('temp_update_interval', 1000)  # ms
		self.fan_update_interval = self.cooling_config.get('fan_update_interval', 100)    # ms

		# Last update times
		self.last_temp_update = datetime.now()
		self.last_fan_update = datetime.now()

		logger.info(f"Cooling simulator initialized with low trigger: {self.low_speed_trigger_temp}°C, "
					f"high trigger: {self.high_speed_trigger_temp}°C")

	async def update(self, current_time: datetime = None, engine_state: bool = None) -> None:
		"""Update the cooling system simulation."""
		if current_time is None:
			current_time = datetime.now()

		if engine_state is not None:
			self.engine_running = engine_state

		# Update temperature
		await self._update_temperature(current_time)

		# Determine fan states
		self._determine_fan_relay_state()

		# Update historical data
		self._update_history(current_time)

		# Simulate temperature changes based on fan states
		await self._simulate_cooling_effects(current_time)

	async def _update_temperature(self, current_time: datetime) -> None:
		"""Update coolant temperature based on engine state and cooling."""
		time_diff = (current_time - self.last_temp_update).total_seconds() * 1000

		if time_diff < self.temp_update_interval:
			return

		self.last_temp_update = current_time

		# Simulate temperature changes based on engine state and cooling
		if self.engine_running:
			# Engine heat generation
			heat_generation = self.load_factor * 2.0  # °C per second at full load

			# Cooling effect from fans
			cooling_effect = 0.0
			if self.low_speed_fan_running:
				cooling_effect += 1.5 * self.cooling_efficiency
			if self.high_speed_fan_running:
				cooling_effect += 3.0 * self.cooling_efficiency

			# Ambient temperature influence
			ambient_influence = (self.ambient_temp - self.current_temp) * 0.01

			# Calculate new temperature
			temp_change = heat_generation - cooling_effect + ambient_influence
			self.current_temp += temp_change * (time_diff / 1000)  # Convert to seconds

			# Apply temperature constraints
			self.current_temp = max(self.min_temp, min(self.max_temp, self.current_temp))
		else:
			# Engine cooling down
			cooling_rate = 0.5  # °C per second when engine off
			self.current_temp -= cooling_rate * (time_diff / 1000)
			self.current_temp = max(self.ambient_temp, self.current_temp)

		# Update rolling average
		self.temp_readings[self.read_index] = self.current_temp
		self.read_index = (self.read_index + 1) % len(self.temp_readings)

	def _determine_fan_relay_state(self) -> None:
		"""Determine which fan relays should be active based on temperature."""
		avg_temp = sum(self.temp_readings) / len(self.temp_readings)

		# Check for overheat condition
		if avg_temp >= self.overheat_temp:
			self.high_speed_fan_running = True
			self.low_speed_fan_running = True
			logger.warning(f"Overheat condition detected: {avg_temp:.1f}°C")
			return

		# High speed fan logic
		if avg_temp >= self.high_speed_trigger_temp:
			if not self.should_buffer_cool_high:
				self.should_buffer_cool_high = True
				logger.info(f"High speed fan buffer activated at {avg_temp:.1f}°C")
		elif avg_temp <= (self.high_speed_trigger_temp - 3):  # 3°C buffer
			self.should_buffer_cool_high = False
			self.high_speed_fan_running = False

		# Low speed fan logic
		if avg_temp >= self.low_speed_trigger_temp:
			if not self.should_buffer_cool_low:
				self.should_buffer_cool_low = True
				logger.info(f"Low speed fan buffer activated at {avg_temp:.1f}°C")
		elif avg_temp <= (self.low_speed_trigger_temp - 2):  # 2°C buffer
			self.should_buffer_cool_low = False
			self.low_speed_fan_running = False

		# Apply buffering
		if self.should_buffer_cool_high:
			self.high_speed_fan_running = True
		if self.should_buffer_cool_low:
			self.low_speed_fan_running = True

		# AC override logic
		if self.ac_state_active:
			self.low_speed_fan_running = True

	def _update_history(self, current_time: datetime) -> None:
		"""Update historical data for monitoring and analysis."""
		# Temperature history
		self.temp_history.append({
			'timestamp': current_time.isoformat(),
			'temperature': self.current_temp,
			'engine_running': self.engine_running
		})

		# Fan state history
		self.fan_state_history.append({
			'timestamp': current_time.isoformat(),
			'low_speed': self.low_speed_fan_running,
			'high_speed': self.high_speed_fan_running,
			'temperature': self.current_temp
		})

		# Keep history size manageable
		if len(self.temp_history) > self.max_history_size:
			self.temp_history.pop(0)
		if len(self.fan_state_history) > self.max_history_size:
			self.fan_state_history.pop(0)

	async def _simulate_cooling_effects(self, current_time: datetime) -> None:
		"""Simulate the cooling effects of active fans."""
		if not self.engine_running:
			return

		# Simulate temperature stabilization when fans are running
		if self.low_speed_fan_running or self.high_speed_fan_running:
			# Temperature should stabilize or decrease
			stabilization_rate = 0.1  # °C per second
			if self.current_temp > self.optimal_temp:
				self.current_temp -= stabilization_rate * (self.temp_update_interval / 1000)

	def set_ac_state(self, ac_active: bool) -> None:
		"""Set the AC system state."""
		self.ac_state_active = ac_active
		if ac_active:
			logger.info("AC system activated - low speed fan enabled")
		else:
			logger.info("AC system deactivated")

	def set_ambient_temperature(self, temp: float) -> None:
		"""Set the ambient temperature for simulation."""
		self.ambient_temp = max(-40, min(60, temp))  # Reasonable range
		logger.info(f"Ambient temperature set to {self.ambient_temp:.1f}°C")

	def set_engine_load(self, load: float) -> None:
		"""Set the engine load factor (0.0 to 1.0)."""
		self.load_factor = max(0.0, min(1.0, load))
		logger.info(f"Engine load factor set to {self.load_factor:.2f}")

	def get_fan_status(self) -> Dict:
		"""Get current fan status and control information."""
		return {
			'low_speed_fan': {
				'running': self.low_speed_fan_running,
				'override': self.low_speed_override,
				'trigger_temp': self.low_speed_trigger_temp,
				'buffer_active': self.should_buffer_cool_low
			},
			'high_speed_fan': {
				'running': self.high_speed_fan_running,
				'override': self.high_speed_override,
				'trigger_temp': self.high_speed_trigger_temp,
				'buffer_active': self.should_buffer_cool_high
			},
			'ac_override': self.ac_state_active,
			'cooling_efficiency': self.cooling_efficiency
		}

	def get_temperature_status(self) -> Dict:
		"""Get current temperature status and thresholds."""
		avg_temp = sum(self.temp_readings) / len(self.temp_readings)

		status = 'normal'
		if avg_temp >= self.overheat_temp:
			status = 'overheat'
		elif avg_temp >= self.high_speed_trigger_temp:
			status = 'high'
		elif avg_temp >= self.low_speed_trigger_temp:
			status = 'elevated'
		elif avg_temp < self.operating_temp:
			status = 'cold'

		return {
			'current_temperature': round(self.current_temp, 1),
			'average_temperature': round(avg_temp, 1),
			'status': status,
			'thresholds': {
				'operating': self.operating_temp,
				'low_speed_trigger': self.low_speed_trigger_temp,
				'high_speed_trigger': self.high_speed_trigger_temp,
				'overheat': self.overheat_temp
			},
			'engine_running': self.engine_running,
			'ambient_temperature': self.ambient_temp
		}

	def get_system_summary(self) -> Dict:
		"""Get comprehensive system status summary."""
		return {
			'enabled': True,
			'temperature': self.get_temperature_status(),
			'fans': self.get_fan_status(),
			'engine': {
				'running': self.engine_running,
				'load_factor': self.load_factor
			},
			'history': {
				'temperature_points': len(self.temp_history),
				'fan_state_points': len(self.fan_state_history)
			}
		}

	async def get_data(self) -> Dict:
		"""Get current cooling data for API responses."""
		return {
			'enabled': True,
			'system_summary': self.get_system_summary(),
			'timestamp': datetime.now().isoformat()
		}
