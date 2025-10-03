#!/usr/bin/env python3
"""
Power Management System for Sensor Hub
Handles battery switching, power monitoring, and power-saving modes
"""

import asyncio
import logging
import time
from datetime import datetime, timedelta
from typing import Dict, List, Optional, Any
import RPi.GPIO as GPIO
import smbus2

logger = logging.getLogger(__name__)

class PowerManager:
	def __init__(self, config: Dict[str, Any]):
		self.config = config
		self.enabled = config.get('enabled', True)

		# Power source configuration
		self.power_source_switch_pin = config.get('power_source_switch_pin', 18)
		self.engine_signal_pin = config.get('engine_signal_pin', 23)
		self.system_led_pin = config.get('system_led_pin', 24)

		# Battery thresholds
		self.low_battery_threshold = config.get('low_battery_threshold', 11.5)
		self.critical_battery_threshold = config.get('critical_battery_threshold', 10.5)

		# Power management settings
		self.sleep_when_engine_off = config.get('sleep_when_engine_off', True)
		self.reduced_polling_when_off = config.get('reduced_polling_when_off', True)
		self.wake_on_engine_start = config.get('wake_on_engine_start', True)
		self.battery_monitoring = config.get('battery_monitoring', True)

		# Current state
		self.current_power_source = "house_battery"  # house_battery or starter_battery
		self.engine_running = False
		self.system_sleeping = False
		self.battery_voltage = 12.6
		self.battery_current = 0.0
		self.power_consumption = 0.0

		# I2C configuration for ADC
		self.i2c_bus = config.get('i2c_bus', 1)
		self.adc_address = config.get('adc_address', 0x48)  # ADS1115 default
		self.i2c = None

		# GPIO setup
		self._setup_gpio()

		# Monitoring task
		self.monitoring_task = None
		self.monitoring_interval = 1.0  # seconds

		# Power history
		self.power_history = []
		self.max_history_size = 1000

		logger.info("Power manager initialized")

	def _setup_gpio(self):
		"""Setup GPIO pins for power management"""
		try:
			GPIO.setup(self.power_source_switch_pin, GPIO.OUT)
			GPIO.setup(self.engine_signal_pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)
			GPIO.setup(self.system_led_pin, GPIO.OUT)

			# Initialize power source to house battery
			GPIO.output(self.power_source_switch_pin, GPIO.LOW)  # House battery
			GPIO.output(self.system_led_pin, GPIO.HIGH)  # System LED on

			logger.info("GPIO pins configured for power management")

		except Exception as e:
			logger.error(f"Error setting up GPIO: {e}")

	def _setup_i2c(self):
		"""Setup I2C communication for ADC"""
		try:
			self.i2c = smbus2.SMBus(self.i2c_bus)
			logger.info(f"I2C bus {self.i2c_bus} initialized")

		except Exception as e:
			logger.error(f"Error setting up I2C: {e}")
			self.i2c = None

	async def start_monitoring(self):
		"""Start power management monitoring"""
		if not self.enabled:
			logger.warning("Power management monitoring is disabled")
			return

		self.monitoring_task = asyncio.create_task(self._monitoring_loop())
		logger.info("Power management monitoring started")

	async def stop_monitoring(self):
		"""Stop power management monitoring"""
		if self.monitoring_task:
			self.monitoring_task.cancel()
			try:
				await self.monitoring_task
			except asyncio.CancelledError:
				pass
			self.monitoring_task = None
			logger.info("Power management monitoring stopped")

	async def _monitoring_loop(self):
		"""Main monitoring loop for power management"""
		while self.enabled:
			try:
				# Read battery voltage and current
				await self._read_battery_status()

				# Check power source
				await self._check_power_source()

				# Update power consumption
				await self._update_power_consumption()

				# Store power data
				await self._store_power_data()

				# Check if we should enter sleep mode
				if self.sleep_when_engine_off and not self.engine_running:
					await self._enter_sleep_mode()

				# Wait for next monitoring cycle
				await asyncio.sleep(self.monitoring_interval)

			except asyncio.CancelledError:
				break
			except Exception as e:
				logger.error(f"Error in power monitoring loop: {e}")
				await asyncio.sleep(5)

	async def _read_battery_status(self):
		"""Read battery voltage and current from ADC"""
		if not self.i2c:
			return

		try:
			# Read voltage from ADC channel 0
			voltage_raw = self._read_adc_channel(0)
			self.battery_voltage = self._convert_adc_to_voltage(voltage_raw)

			# Read current from ADC channel 1
			current_raw = self._read_adc_channel(1)
			self.battery_current = self._convert_adc_to_current(current_raw)

			# Calculate power consumption
			self.power_consumption = self.battery_voltage * self.battery_current

		except Exception as e:
			logger.error(f"Error reading battery status: {e}")

	def _read_adc_channel(self, channel: int) -> int:
		"""Read ADC channel value"""
		if not self.i2c:
			return 0

		try:
			# ADS1115 configuration
			config = 0x8583 | (channel << 12)  # Single shot, AIN0, ±4.096V, 128 SPS
			self.i2c.write_i2c_block_data(self.adc_address, 1, [config >> 8, config & 0xFF])

			# Wait for conversion
			time.sleep(0.01)

			# Read result
			data = self.i2c.read_i2c_block_data(self.adc_address, 0, 2)
			value = (data[0] << 8) | data[1]
			return value

		except Exception as e:
			logger.error(f"Error reading ADC channel {channel}: {e}")
			return 0

	def _convert_adc_to_voltage(self, adc_value: int) -> float:
		"""Convert ADC value to voltage"""
		# ADS1115 with ±4.096V range
		voltage = (adc_value / 32767.0) * 4.096
		# Apply voltage divider ratio (if using voltage divider)
		voltage_divider_ratio = self.config.get('voltage_divider_ratio', 3.0)
		return voltage * voltage_divider_ratio

	def _convert_adc_to_current(self, adc_value: int) -> float:
		"""Convert ADC value to current"""
		# ACS712 current sensor (30A version)
		# 0A = 2.5V, 30A = 3.5V
		voltage = (adc_value / 32767.0) * 4.096
		current = (voltage - 2.5) * 30.0  # 30A range
		return max(0, current)  # Current cannot be negative

	async def _check_power_source(self):
		"""Check and update power source"""
		try:
			power_source_state = GPIO.input(self.power_source_switch_pin)
			new_power_source = "starter_battery" if power_source_state else "house_battery"

			if new_power_source != self.current_power_source:
				self.current_power_source = new_power_source
				logger.info(f"Power source changed to: {self.current_power_source}")

		except Exception as e:
			logger.error(f"Error checking power source: {e}")

	async def _update_power_consumption(self):
		"""Update power consumption calculations"""
		try:
			# Calculate power consumption based on current draw
			base_consumption = 2.0  # Base system consumption in watts
			sensor_consumption = 1.5  # Sensor consumption in watts
			communication_consumption = 0.5  # Communication consumption in watts

			total_consumption = base_consumption + sensor_consumption + communication_consumption

			# Add dynamic consumption based on system state
			if self.engine_running:
				total_consumption += 1.0  # Additional power when engine running

			if self.system_sleeping:
				total_consumption *= 0.3  # Reduce power in sleep mode

			self.power_consumption = total_consumption

		except Exception as e:
			logger.error(f"Error updating power consumption: {e}")

	async def _store_power_data(self):
		"""Store power data in history"""
		try:
			power_data = {
				'timestamp': datetime.now().isoformat(),
				'battery_voltage': round(self.battery_voltage, 2),
				'battery_current': round(self.battery_current, 3),
				'power_consumption': round(self.power_consumption, 2),
				'power_source': self.current_power_source,
				'engine_running': self.engine_running,
				'system_sleeping': self.system_sleeping
			}

			self.power_history.append(power_data)

			# Limit history size
			if len(self.power_history) > self.max_history_size:
				self.power_history.pop(0)

		except Exception as e:
			logger.error(f"Error storing power data: {e}")

	async def _enter_sleep_mode(self):
		"""Enter power-saving sleep mode"""
		if self.system_sleeping:
			return

		try:
			logger.info("Entering power-saving sleep mode")
			self.system_sleeping = True

			# Reduce monitoring frequency
			self.monitoring_interval = 5.0  # 5 seconds instead of 1

			# Dim system LED
			GPIO.output(self.system_led_pin, GPIO.LOW)

			# Notify other components about sleep mode
			await self._notify_sleep_mode(True)

		except Exception as e:
			logger.error(f"Error entering sleep mode: {e}")

	async def _exit_sleep_mode(self):
		"""Exit power-saving sleep mode"""
		if not self.system_sleeping:
			return

		try:
			logger.info("Exiting power-saving sleep mode")
			self.system_sleeping = False

			# Restore normal monitoring frequency
			self.monitoring_interval = 1.0

			# Restore system LED
			GPIO.output(self.system_led_pin, GPIO.HIGH)

			# Notify other components about wake mode
			await self._notify_sleep_mode(False)

		except Exception as e:
			logger.error(f"Error exiting sleep mode: {e}")

	async def _notify_sleep_mode(self, sleeping: bool):
		"""Notify other components about sleep mode changes"""
		# This would typically notify other managers about power state changes
		logger.debug(f"System sleep mode: {'enabled' if sleeping else 'disabled'}")

	def engine_state_changed(self, engine_running: bool):
		"""Handle engine state changes"""
		try:
			old_state = self.engine_running
			self.engine_running = engine_running

			if engine_running and not old_state:
				# Engine started
				logger.info("Engine started - exiting sleep mode")
				asyncio.create_task(self._exit_sleep_mode())

			elif not engine_running and old_state:
				# Engine stopped
				logger.info("Engine stopped - considering sleep mode")
				if self.sleep_when_engine_off:
					asyncio.create_task(self._enter_sleep_mode())

		except Exception as e:
			logger.error(f"Error handling engine state change: {e}")

	def power_source_changed(self, power_source: str):
		"""Handle power source changes"""
		try:
			old_source = self.current_power_source
			self.current_power_source = power_source

			if power_source != old_source:
				logger.info(f"Power source changed from {old_source} to {power_source}")

				# Update GPIO state
				if power_source == "starter_battery":
					GPIO.output(self.power_source_switch_pin, GPIO.HIGH)
				else:
					GPIO.output(self.power_source_switch_pin, GPIO.LOW)

		except Exception as e:
			logger.error(f"Error handling power source change: {e}")

	def get_battery_status(self) -> Dict[str, Any]:
		"""Get current battery status"""
		status = "normal"
		if self.battery_voltage <= self.critical_battery_threshold:
			status = "critical"
		elif self.battery_voltage <= self.low_battery_threshold:
			status = "low"

		return {
			'voltage': round(self.battery_voltage, 2),
			'current': round(self.battery_current, 3),
			'power_consumption': round(self.power_consumption, 2),
			'status': status,
			'thresholds': {
				'low': self.low_battery_threshold,
				'critical': self.critical_battery_threshold
			}
		}

	def get_power_status(self) -> Dict[str, Any]:
		"""Get current power system status"""
		return {
			'power_source': self.current_power_source,
			'engine_running': self.engine_running,
			'system_sleeping': self.system_sleeping,
			'battery': self.get_battery_status(),
			'monitoring_interval': self.monitoring_interval,
			'enabled': self.enabled
		}

	async def get_data(self) -> Dict[str, Any]:
		"""Get current power data for API responses"""
		if not self.enabled:
			return {
				'enabled': False,
				'error': 'Power manager disabled'
			}

		return {
			'enabled': True,
			'power_status': self.get_power_status(),
			'recent_history': self.power_history[-10:],  # Last 10 readings
			'timestamp': datetime.now().isoformat()
		}

	def cleanup(self):
		"""Cleanup GPIO and I2C resources"""
		try:
			if self.i2c:
				self.i2c.close()
				self.i2c = None

			# GPIO cleanup is handled by main applicationme 
			logger.info("Power manager cleanup completed")

		except Exception as e:
			logger.error(f"Error during power manager cleanup: {e}")
