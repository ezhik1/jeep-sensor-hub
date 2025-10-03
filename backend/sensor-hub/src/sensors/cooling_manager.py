#!/usr/bin/env python3
"""
Engine Cooling Management System
Handles dual-speed fan control, temperature monitoring, and smart buffering logic
"""

import asyncio
import logging
import time
from datetime import datetime, timedelta
from typing import Dict, List, Optional, Any
import RPi.GPIO as GPIO

logger = logging.getLogger(__name__)

class CoolingManager:
	def __init__(self, config: Dict[str, Any]):
		self.config = config
		self.enabled = config.get('enabled', True)

		# Temperature thresholds (in Celsius)
		self.low_speed_trigger_temp = config.get('low_speed_trigger_temp', 93)
		self.high_speed_trigger_temp = config.get('high_speed_trigger_temp', 105)
		self.min_temp = config.get('min_temp', 60.0)
		self.max_temp = config.get('max_temp', 121.11)
		self.overheat_temp = config.get('overheat_temp', 112)
		self.operating_temp = config.get('operating_temp', 90.5)

		# Timing configuration
		self.temp_update_interval = config.get('temp_update_interval', 1000)  # ms
		self.fan_update_interval = config.get('fan_update_interval', 100)    # ms

		# Current state
		self.current_temperature = 79.3  # Start at cold temperature
		self.target_temperature = 79.3
		self.low_speed_fan_running = False
		self.high_speed_fan_running = False
		self.ac_state_active = False
		self.low_speed_override = False
		self.high_speed_override = False
		self.buffer_cooling = False
		self.should_buffer_cool_low = False
		self.should_buffer_cool_high = False
		self.engine_running = False
		self.load_factor = 0.5
		self.ambient_temperature = config.get('ambient_temp', 25.0)
		self.cooling_efficiency = config.get('cooling_efficiency', 0.8)

		# GPIO pins for fan control
		self.low_speed_pin = config.get('low_speed_pin', 5)
		self.high_speed_pin = config.get('high_speed_pin', 6)

		# Temperature sensor configuration
		self.temp_sensor_pin = config.get('temp_sensor_pin', 17)
		self.temp_sensor_type = config.get('temp_sensor_type', 'DHT22')

		# History tracking
		self.temperature_history = []
		self.fan_operation_history = []
		self.max_history_size = 1000

		# Initialize GPIO
		self._setup_gpio()

		# Start monitoring tasks
		self.monitoring_task = None
		self.fan_control_task = None

		logger.info(f"Cooling manager initialized with low trigger: {self.low_speed_trigger_temp}°C, high trigger: {self.high_speed_trigger_temp}°C")

	def _setup_gpio(self):
		"""Setup GPIO pins for fan control"""
		try:
			GPIO.setup(self.low_speed_pin, GPIO.OUT)
			GPIO.setup(self.high_speed_pin, GPIO.OUT)

			# Initialize fans to off
			GPIO.output(self.low_speed_pin, GPIO.LOW)
			GPIO.output(self.high_speed_pin, GPIO.LOW)

			logger.info("GPIO pins configured for fan control")

		except Exception as e:
			logger.error(f"Error setting up GPIO: {e}")

	async def start_monitoring(self):
		"""Start the cooling system monitoring"""
		if not self.enabled:
			logger.warning("Cooling system is disabled")
			return

		self.monitoring_task = asyncio.create_task(self._temperature_monitoring_loop())
		self.fan_control_task = asyncio.create_task(self._fan_control_loop())

		logger.info("Cooling system monitoring started")

	async def stop_monitoring(self):
		"""Stop the cooling system monitoring"""
		if self.monitoring_task:
			self.monitoring_task.cancel()
			try:
				await self.monitoring_task
			except asyncio.CancelledError:
				pass
			self.monitoring_task = None

		if self.fan_control_task:
			self.fan_control_task.cancel()
			try:
				await self.fan_control_task
			except asyncio.CancelledError:
				pass
			self.fan_control_task = None

		# Turn off all fans
		self._turn_off_all_fans()

		logger.info("Cooling system monitoring stopped")

	async def _temperature_monitoring_loop(self):
		"""Main temperature monitoring loop"""
		while self.enabled:
			try:
				# Update temperature
				await self._update_temperature()

				# Store temperature history
				self._store_temperature_history()

				# Wait for next update
				await asyncio.sleep(self.temp_update_interval / 1000.0)

			except asyncio.CancelledError:
				break
			except Exception as e:
				logger.error(f"Error in temperature monitoring loop: {e}")
				await asyncio.sleep(5)

	async def _fan_control_loop(self):
		"""Main fan control loop"""
		while self.enabled:
			try:
				# Determine fan states
				await self._determine_fan_states()

				# Apply fan control
				await self._apply_fan_control()

				# Store fan operation history
				self._store_fan_operation_history()

				# Wait for next update
				await asyncio.sleep(self.fan_update_interval / 1000.0)

			except asyncio.CancelledError:
				break
			except Exception as e:
				logger.error(f"Error in fan control loop: {e}")
				await asyncio.sleep(1)

	async def _update_temperature(self):
		"""Update current temperature based on engine state and load"""
		try:
			if self.engine_running:
				# Calculate target temperature based on engine load and ambient
				self.target_temperature = self._calculate_target_temperature()

				# Gradually move current temperature toward target
				temperature_diff = self.target_temperature - self.current_temperature
				change_rate = 0.1  # °C per update cycle

				if abs(temperature_diff) > change_rate:
					if temperature_diff > 0:
						self.current_temperature += change_rate
					else:
						self.current_temperature -= change_rate

				# Apply cooling effects if fans are running
				if self.low_speed_fan_running or self.high_speed_fan_running:
					await self._simulate_cooling_effects()

			else:
				# Engine off - cool down toward ambient
				if self.current_temperature > self.ambient_temperature:
					cooling_rate = 0.05  # Slower cooling when engine off
					self.current_temperature = max(
						self.ambient_temperature,
						self.current_temperature - cooling_rate
					)

			# Ensure temperature stays within bounds
			self.current_temperature = max(self.min_temp, min(self.max_temp, self.current_temperature))

		except Exception as e:
			logger.error(f"Error updating temperature: {e}")

	def _calculate_target_temperature(self) -> float:
		"""Calculate target temperature based on engine load and ambient"""
		try:
			# Base operating temperature
			base_temp = self.operating_temp

			# Adjust for engine load
			load_adjustment = self.load_factor * 15.0  # Up to 15°C increase at full load

			# Adjust for ambient temperature
			ambient_adjustment = (self.ambient_temperature - 25.0) * 0.3

			target_temp = base_temp + load_adjustment + ambient_adjustment

			# Ensure target is within reasonable bounds
			return max(self.min_temp, min(self.max_temp, target_temp))

		except Exception as e:
			logger.error(f"Error calculating target temperature: {e}")
			return self.operating_temp

	async def _simulate_cooling_effects(self):
		"""Simulate cooling effects from running fans"""
		try:
			cooling_rate = 0.0

			if self.high_speed_fan_running:
				cooling_rate += 0.3 * self.cooling_efficiency
			elif self.low_speed_fan_running:
				cooling_rate += 0.15 * self.cooling_efficiency

			# Apply cooling
			if cooling_rate > 0:
				self.current_temperature = max(
					self.ambient_temperature,
					self.current_temperature - cooling_rate
				)

		except Exception as e:
			logger.error(f"Error simulating cooling effects: {e}")

	async def _determine_fan_states(self):
		"""Determine which fans should be running"""
		try:
			# Check for overrides first
			if self.low_speed_override:
				self.should_buffer_cool_low = True
				self.should_buffer_cool_high = False
				return

			if self.high_speed_override:
				self.should_buffer_cool_low = False
				self.should_buffer_cool_high = True
				return

			# Normal temperature-based control
			if self.current_temperature >= self.overheat_temp:
				# Critical overheat - both fans on
				self.should_buffer_cool_low = True
				self.should_buffer_cool_high = True
				logger.warning(f"Critical overheat detected: {self.current_temperature:.1f}°C")

			elif self.current_temperature >= self.high_speed_trigger_temp:
				# High temperature - high speed fan
				self.should_buffer_cool_low = False
				self.should_buffer_cool_high = True

			elif self.current_temperature >= self.low_speed_trigger_temp:
				# Moderate temperature - low speed fan
				self.should_buffer_cool_low = True
				self.should_buffer_cool_high = False

			else:
				# Normal temperature - no fans needed
				self.should_buffer_cool_low = False
				self.should_buffer_cool_high = False

			# AC override - if AC is on, ensure low speed fan is running
			if self.ac_state_active and not self.should_buffer_cool_low:
				self.should_buffer_cool_low = True

		except Exception as e:
			logger.error(f"Error determining fan states: {e}")

	async def _apply_fan_control(self):
		"""Apply the determined fan states"""
		try:
			# Apply low speed fan
			if self.should_buffer_cool_low and not self.low_speed_fan_running:
				self._turn_on_low_speed_fan()
			elif not self.should_buffer_cool_low and self.low_speed_fan_running:
				self._turn_off_low_speed_fan()

			# Apply high speed fan
			if self.should_buffer_cool_high and not self.high_speed_fan_running:
				self._turn_on_high_speed_fan()
			elif not self.should_buffer_cool_high and self.high_speed_fan_running:
				self._turn_off_high_speed_fan()

		except Exception as e:
			logger.error(f"Error applying fan control: {e}")

	def _turn_on_low_speed_fan(self):
		"""Turn on low speed fan"""
		try:
			GPIO.output(self.low_speed_pin, GPIO.HIGH)
			self.low_speed_fan_running = True
			logger.debug("Low speed fan turned ON")

		except Exception as e:
			logger.error(f"Error turning on low speed fan: {e}")

	def _turn_off_low_speed_fan(self):
		"""Turn off low speed fan"""
		try:
			GPIO.output(self.low_speed_pin, GPIO.LOW)
			self.low_speed_fan_running = False
			logger.debug("Low speed fan turned OFF")

		except Exception as e:
			logger.error(f"Error turning off low speed fan: {e}")

	def _turn_on_high_speed_fan(self):
		"""Turn on high speed fan"""
		try:
			GPIO.output(self.high_speed_pin, GPIO.HIGH)
			self.high_speed_fan_running = True
			logger.debug("High speed fan turned ON")

		except Exception as e:
			logger.error(f"Error turning on high speed fan: {e}")

	def _turn_off_high_speed_fan(self):
		"""Turn off high speed fan"""
		try:
			GPIO.output(self.high_speed_pin, GPIO.LOW)
			self.high_speed_fan_running = False
			logger.debug("High speed fan turned OFF")

		except Exception as e:
			logger.error(f"Error turning off high speed fan: {e}")

	def _turn_off_all_fans(self):
		"""Turn off all fans"""
		try:
			self._turn_off_low_speed_fan()
			self._turn_off_high_speed_fan()

		except Exception as e:
			logger.error(f"Error turning off all fans: {e}")

	def _store_temperature_history(self):
		"""Store temperature reading in history"""
		try:
			reading = {
				'timestamp': datetime.now(),
				'temperature': self.current_temperature,
				'target_temperature': self.target_temperature,
				'engine_running': self.engine_running,
				'load_factor': self.load_factor,
				'ambient_temperature': self.ambient_temperature
			}

			self.temperature_history.append(reading)

			# Limit history size
			if len(self.temperature_history) > self.max_history_size:
				self.temperature_history.pop(0)

		except Exception as e:
			logger.error(f"Error storing temperature history: {e}")

	def _store_fan_operation_history(self):
		"""Store fan operation in history"""
		try:
			operation = {
				'timestamp': datetime.now(),
				'low_speed_fan': self.low_speed_fan_running,
				'high_speed_fan': self.high_speed_fan_running,
				'temperature': self.current_temperature,
				'ac_state': self.ac_state_active,
				'low_override': self.low_speed_override,
				'high_override': self.high_speed_override
			}

			self.fan_operation_history.append(operation)

			# Limit history size
			if len(self.fan_operation_history) > self.max_history_size:
				self.fan_operation_history.pop(0)

		except Exception as e:
			logger.error(f"Error storing fan operation history: {e}")

	# Public API methods
	def set_engine_state(self, running: bool):
		"""Set engine running state"""
		try:
			old_state = self.engine_running
			self.engine_running = running

			if running and not old_state:
				logger.info("Engine started - cooling system active")
			elif not running and old_state:
				logger.info("Engine stopped - cooling system cooling down")

		except Exception as e:
			logger.error(f"Error setting engine state: {e}")

	def set_load_factor(self, load: float):
		"""Set engine load factor (0.0 to 1.0)"""
		try:
			self.load_factor = max(0.0, min(1.0, load))
			logger.debug(f"Engine load factor set to: {self.load_factor:.2f}")

		except Exception as e:
			logger.error(f"Error setting load factor: {e}")

	def set_ac_state(self, active: bool):
		"""Set AC system state"""
		try:
			old_state = self.ac_state_active
			self.ac_state_active = active

			if active and not old_state:
				logger.info("AC system activated - low speed fan override enabled")
			elif not active and old_state:
				logger.info("AC system deactivated - normal fan control restored")

		except Exception as e:
			logger.error(f"Error setting AC state: {e}")

	def set_ambient_temperature(self, temp: float):
		"""Set ambient temperature"""
		try:
			self.ambient_temperature = max(-40.0, min(60.0, temp))
			logger.debug(f"Ambient temperature set to: {self.ambient_temperature:.1f}°C")

		except Exception as e:
			logger.error(f"Error setting ambient temperature: {e}")

	def override_fan_control(self, low_speed: bool = False, high_speed: bool = False):
		"""Override fan control (for testing or emergency)"""
		try:
			self.low_speed_override = low_speed
			self.high_speed_override = high_speed

			if low_speed or high_speed:
				logger.warning(f"Fan control override: low={low_speed}, high={high_speed}")
			else:
				logger.info("Fan control override cleared - normal operation restored")

		except Exception as e:
			logger.error(f"Error setting fan override: {e}")

	def get_current_status(self) -> Dict[str, Any]:
		"""Get current cooling system status"""
		try:
			return {
				'temperature': {
					'current': round(self.current_temperature, 1),
					'target': round(self.target_temperature, 1),
					'ambient': round(self.ambient_temperature, 1)
				},
				'fans': {
					'low_speed': self.low_speed_fan_running,
					'high_speed': self.high_speed_fan_running,
					'low_override': self.low_speed_override,
					'high_override': self.high_speed_override
				},
				'system': {
					'engine_running': self.engine_running,
					'ac_active': self.ac_state_active,
					'load_factor': round(self.load_factor, 2),
					'cooling_efficiency': round(self.cooling_efficiency, 2)
				},
				'thresholds': {
					'low_speed_trigger': self.low_speed_trigger_temp,
					'high_speed_trigger': self.high_speed_trigger_temp,
					'overheat': self.overheat_temp,
					'operating': self.operating_temp
				}
			}

		except Exception as e:
			logger.error(f"Error getting current status: {e}")
			return {}

	def get_temperature_history(self, limit: int = 100) -> List[Dict]:
		"""Get temperature history"""
		try:
			history = self.temperature_history[-limit:] if len(self.temperature_history) > limit else self.temperature_history
			return [
				{
					'timestamp': entry['timestamp'].isoformat(),
					'temperature': entry['temperature'],
					'target_temperature': entry['target_temperature'],
					'engine_running': entry['engine_running'],
					'load_factor': entry['load_factor'],
					'ambient_temperature': entry['ambient_temperature']
				}
				for entry in history
			]

		except Exception as e:
			logger.error(f"Error getting temperature history: {e}")
			return []

	def get_fan_operation_history(self, limit: int = 100) -> List[Dict]:
		"""Get fan operation history"""
		try:
			history = self.fan_operation_history[-limit:] if len(self.fan_operation_history) > limit else self.fan_operation_history
			return [
				{
					'timestamp': entry['timestamp'].isoformat(),
					'low_speed_fan': entry['low_speed_fan'],
					'high_speed_fan': entry['high_speed_fan'],
					'temperature': entry['temperature'],
					'ac_state': entry['ac_state'],
					'low_override': entry['low_override'],
					'high_override': entry['high_override']
				}
				for entry in history
			]

		except Exception as e:
			logger.error(f"Error getting fan operation history: {e}")
			return []

	async def update(self, timestamp: datetime):
		"""Update cooling manager (called by main loop)"""
		if not self.enabled:
			return

		try:
			# This method is called by the main loop
			# The actual monitoring happens in the background tasks
			pass

		except Exception as e:
			logger.error(f"Error updating cooling manager: {e}")

	async def get_data(self) -> Dict[str, Any]:
		"""Get current cooling data for API responses"""
		if not self.enabled:
			return {
				'enabled': False,
				'error': 'Cooling manager disabled'
			}

		return {
			'enabled': True,
			'current_status': self.get_current_status(),
			'temperature_history': self.get_temperature_history(50),
			'fan_operation_history': self.get_fan_operation_history(50),
			'timestamp': datetime.now().isoformat()
		}

	def cleanup(self):
		"""Cleanup resources"""
		try:
			# Turn off all fans
			self._turn_off_all_fans()

			logger.info("Cooling manager cleanup completed")

		except Exception as e:
			logger.error(f"Error during cooling manager cleanup: {e}")
