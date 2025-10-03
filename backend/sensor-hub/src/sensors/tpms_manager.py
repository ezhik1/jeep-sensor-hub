#!/usr/bin/env python3
"""
TPMS Manager for Sensor Hub
Handles tire pressure monitoring via CC1101 radio module
"""

import asyncio
import logging
import time
from datetime import datetime, timedelta
from typing import Dict, List, Optional, Any
import RPi.GPIO as GPIO
import spidev

logger = logging.getLogger(__name__)

class TPMSManager:
	def __init__(self, config: Dict[str, Any]):
		self.config = config
		self.enabled = config.get('enabled', True)

		# SPI configuration for CC1101
		self.spi_bus = config.get('spi_bus', 0)
		self.spi_device = config.get('spi_device', 0)
		self.cs_pin = config.get('cs_pin', 8)  # GPIO8 (CE0)
		self.gdo0_pin = config.get('gdo0_pin', 25)  # GPIO25 for interrupt
		self.gdo2_pin = config.get('gdo2_pin', 26)  # GPIO26 for data

		# Radio configuration
		self.frequency = config.get('frequency', 433.92)  # MHz
		self.power = config.get('power', 10)  # dBm

		# Sensor configuration
		self.sensors = config.get('sensors', [])
		self.sensor_data = {}

		# Initialize sensor data
		for sensor in self.sensors:
			sensor_id = sensor['id']
			self.sensor_data[sensor_id] = {
				'id': sensor_id,
				'name': sensor.get('name', 'Unknown'),
				'position': sensor.get('position', 'Unknown'),
				'pressure': sensor.get('pressure', 32.0),
				'temperature': sensor.get('temperature', 25.0),
				'battery_level': sensor.get('battery_level', 85),
				'last_update': datetime.now(),
				'low_threshold': sensor.get('low_threshold', 20),
				'high_threshold': sensor.get('high_threshold', 40),
				'target_pressure': sensor.get('target_pressure', 32.0),
				'pressure_range': sensor.get('pressure_range', [28.0, 36.0]),
				'temp_range': sensor.get('temp_range', [40.0, 120.0])
			}

		# SPI and GPIO setup
		self.spi = None
		self._setup_hardware()

		# Monitoring task
		self.monitoring_task = None
		self.last_scan_time = datetime.now()
		self.scan_interval = 5.0  # seconds

		logger.info(f"TPMS manager initialized with {len(self.sensors)} sensors")

	def _setup_hardware(self):
		"""Setup SPI and GPIO for CC1101 communication"""
		try:
			# Setup GPIO
			GPIO.setup(self.cs_pin, GPIO.OUT)
			GPIO.setup(self.gdo0_pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)
			GPIO.setup(self.gdo2_pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)

			# Initialize SPI
			self.spi = spidev.SpiDev()
			self.spi.open(self.spi_bus, self.spi_device)
			self.spi.max_speed_hz = 1000000  # 1 MHz
			self.spi.mode = 0

			# Initialize CC1101
			self._init_cc1101()

			logger.info("TPMS hardware setup completed")

		except Exception as e:
			logger.error(f"Error setting up TPMS hardware: {e}")
			self.spi = None

	def _init_cc1101(self):
		"""Initialize CC1101 radio module"""
		try:
			# CC1101 initialization sequence
			# Reset
			self._write_register(0x30, 0x80)  # SRES
			time.sleep(0.1)

			# Configure for 433 MHz operation
			self._write_register(0x0C, 0x0D)  # FSCTRL1
			self._write_register(0x0D, 0x00)  # FSCTRL0
			self._write_register(0x10, 0x10)  # FREQ2
			self._write_register(0x11, 0xA7)  # FREQ1
			self._write_register(0x12, 0x62)  # FREQ0

			# Configure data rate and modulation
			self._write_register(0x13, 0x3B)  # MDMCFG4
			self._write_register(0x14, 0x93)  # MDMCFG3
			self._write_register(0x15, 0x22)  # MDMCFG2
			self._write_register(0x16, 0x03)  # MDMCFG1
			self._write_register(0x17, 0x22)  # MDMCFG0

			# Configure packet handling
			self._write_register(0x18, 0x00)  # DEVIATN
			self._write_register(0x19, 0x47)  # MCSM2
			self._write_register(0x1A, 0x30)  # MCSM1
			self._write_register(0x1B, 0x07)  # MCSM0

			# Configure FIFO and interrupts
			self._write_register(0x1C, 0x00)  # FOCCFG
			self._write_register(0x1D, 0x1D)  # BSCFG
			self._write_register(0x1E, 0x1C)  # AGCCTRL2
			self._write_register(0x1F, 0xC7)  # AGCCTRL1
			self._write_register(0x20, 0x00)  # AGCCTRL0

			# Configure wake-on-radio
			self._write_register(0x21, 0x00)  # WOREVT1
			self._write_register(0x22, 0x00)  # WOREVT0
			self._write_register(0x23, 0x00)  # WORCTRL

			# Configure front-end
			self._write_register(0x24, 0xB6)  # FREND1
			self._write_register(0x25, 0x10)  # FREND0

			# Configure test registers
			self._write_register(0x26, 0x18)  # FSCAL3
			self._write_register(0x27, 0x1D)  # FSCAL2
			self._write_register(0x28, 0x7E)  # FSCAL1
			self._write_register(0x29, 0x6F)  # FSCAL0

			# Configure RC oscillator
			self._write_register(0x2A, 0x41)  # RCCTRL1
			self._write_register(0x2B, 0x00)  # RCCTRL0

			# Set to receive mode
			self._set_receive_mode()

			logger.info("CC1101 initialized successfully")

		except Exception as e:
			logger.error(f"Error initializing CC1101: {e}")

	def _write_register(self, address: int, value: int):
		"""Write to CC1101 register"""
		try:
			GPIO.output(self.cs_pin, GPIO.LOW)
			time.sleep(0.001)  # 1ms delay

			# Send write command and address
			self.spi.writebytes([0x40 | address, value])

			GPIO.output(self.cs_pin, GPIO.HIGH)
			time.sleep(0.001)  # 1ms delay

		except Exception as e:
			logger.error(f"Error writing to CC1101 register {address:02X}: {e}")

	def _read_register(self, address: int) -> int:
		"""Read from CC1101 register"""
		try:
			GPIO.output(self.cs_pin, GPIO.LOW)
			time.sleep(0.001)  # 1ms delay

			# Send read command and address
			self.spi.writebytes([0x80 | address, 0x00])
			response = self.spi.readbytes(1)

			GPIO.output(self.cs_pin, GPIO.HIGH)
			time.sleep(0.001)  # 1ms delay

			return response[0] if response else 0

		except Exception as e:
			logger.error(f"Error reading from CC1101 register {address:02X}: {e}")
			return 0

	def _set_receive_mode(self):
		"""Set CC1101 to receive mode"""
		try:
			# Set to receive mode
			self._write_register(0x30, 0x0D)  # SRX
			time.sleep(0.01)  # 10ms delay

		except Exception as e:
			logger.error(f"Error setting receive mode: {e}")

	def _set_transmit_mode(self):
		"""Set CC1101 to transmit mode"""
		try:
			# Set to transmit mode
			self._write_register(0x30, 0x0C)  # STX
			time.sleep(0.01)  # 10ms delay

		except Exception as e:
			logger.error(f"Error setting transmit mode: {e}")

	async def start_monitoring(self):
		"""Start TPMS monitoring"""
		if not self.enabled:
			logger.warning("TPMS monitoring is disabled")
			return

		if not self.spi:
			logger.error("Cannot start monitoring - hardware not initialized")
			return

		self.monitoring_task = asyncio.create_task(self._monitoring_loop())
		logger.info("TPMS monitoring started")

	async def stop_monitoring(self):
		"""Stop TPMS monitoring"""
		if self.monitoring_task:
			self.monitoring_task.cancel()
			try:
				await self.monitoring_task
			except asyncio.CancelledError:
				pass
			self.monitoring_task = None
			logger.info("TPMS monitoring stopped")

	async def _monitoring_loop(self):
		"""Main monitoring loop for TPMS sensors"""
		while self.enabled:
			try:
				# Check for received data
				await self._check_for_data()

				# Update sensor status
				await self._update_sensor_status()

				# Wait for next scan
				await asyncio.sleep(self.scan_interval)

			except asyncio.CancelledError:
				break
			except Exception as e:
				logger.error(f"Error in TPMS monitoring loop: {e}")
				await asyncio.sleep(5)

	async def _check_for_data(self):
		"""Check for received TPMS data"""
		try:
			# Check if data is available
			if GPIO.input(self.gdo0_pin) == GPIO.HIGH:
				# Read received data
				data = self._read_received_data()
				if data:
					await self._process_received_data(data)

		except Exception as e:
			logger.error(f"Error checking for TPMS data: {e}")

	def _read_received_data(self) -> Optional[bytes]:
		"""Read received data from CC1101"""
		try:
			# Check packet length
			packet_length = self._read_register(0x3B)  # RXBYTES
			if packet_length == 0:
				return None

			# Read packet data
			GPIO.output(self.cs_pin, GPIO.LOW)
			time.sleep(0.001)

			# Send read command for RX FIFO
			self.spi.writebytes([0x3F, 0x00])
			data = self.spi.readbytes(packet_length)

			GPIO.output(self.cs_pin, GPIO.HIGH)
			time.sleep(0.001)

			return bytes(data)

		except Exception as e:
			logger.error(f"Error reading received data: {e}")
			return None

	async def _process_received_data(self, data: bytes):
		"""Process received TPMS data"""
		try:
			if len(data) < 8:  # Minimum packet size
				return

			# Parse TPMS packet (example format - adjust based on your sensors)
			sensor_id = data[0]
			pressure_raw = (data[1] << 8) | data[2]
			temperature_raw = (data[3] << 8) | data[4]
			battery_raw = data[5]
			flags = data[6]
			checksum = data[7]

			# Verify checksum
			calculated_checksum = sum(data[:-1]) & 0xFF
			if calculated_checksum != checksum:
				logger.warning(f"TPMS packet checksum mismatch for sensor {sensor_id}")
				return

			# Convert raw values
			pressure = pressure_raw / 100.0  # PSI
			temperature = temperature_raw / 10.0  # °C
			battery_level = battery_raw  # Percentage

			# Update sensor data
			if sensor_id in self.sensor_data:
				self.sensor_data[sensor_id].update({
					'pressure': pressure,
					'temperature': temperature,
					'battery_level': battery_level,
					'last_update': datetime.now()
				})

				logger.debug(f"Updated TPMS sensor {sensor_id}: {pressure:.1f} PSI, {temperature:.1f}°C, {battery_level}%")

			else:
				logger.info(f"Received data from unknown TPMS sensor: {sensor_id}")

		except Exception as e:
			logger.error(f"Error processing TPMS data: {e}")

	async def _update_sensor_status(self):
		"""Update sensor status and check for alerts"""
		try:
			current_time = datetime.now()

			for sensor_id, sensor_info in self.sensor_data.items():
				# Check for stale data
				time_since_update = (current_time - sensor_info['last_update']).total_seconds()
				if time_since_update > 300:  # 5 minutes
					sensor_info['status'] = 'stale'
					logger.warning(f"TPMS sensor {sensor_id} data is stale ({time_since_update:.0f}s)")

				# Check pressure thresholds
				pressure = sensor_info['pressure']
				if pressure < sensor_info['low_threshold']:
					sensor_info['status'] = 'low_pressure'
					logger.warning(f"TPMS sensor {sensor_id} low pressure: {pressure:.1f} PSI")
				elif pressure > sensor_info['high_threshold']:
					sensor_info['status'] = 'high_pressure'
					logger.warning(f"TPMS sensor {sensor_id} high pressure: {pressure:.1f} PSI")
				else:
					sensor_info['status'] = 'normal'

				# Check battery level
				battery = sensor_info['battery_level']
				if battery < 20:
					sensor_info['status'] = 'low_battery'
					logger.warning(f"TPMS sensor {sensor_id} low battery: {battery}%")

		except Exception as e:
			logger.error(f"Error updating sensor status: {e}")

	async def update(self, timestamp: datetime):
		"""Update TPMS manager (called by main loop)"""
		if not self.enabled:
			return

		try:
			# Check if we need to scan for new sensors
			time_since_scan = (timestamp - self.last_scan_time).total_seconds()
			if time_since_scan > 60:  # Scan every minute
				await self._scan_for_new_sensors()
				self.last_scan_time = timestamp

		except Exception as e:
			logger.error(f"Error updating TPMS manager: {e}")

	async def _scan_for_new_sensors(self):
		"""Scan for new TPMS sensors"""
		try:
			logger.debug("Scanning for new TPMS sensors...")

			# This would typically involve sending a discovery command
			# or listening for sensor advertisements
			# For now, we'll just log the scan

		except Exception as e:
			logger.error(f"Error scanning for new sensors: {e}")

	def add_sensor(self, sensor_config: Dict[str, Any]):
		"""Add a new TPMS sensor"""
		try:
			sensor_id = sensor_config['id']
			if sensor_id in self.sensor_data:
				logger.warning(f"TPMS sensor {sensor_id} already exists")
				return

			self.sensor_data[sensor_id] = {
				'id': sensor_id,
				'name': sensor_config.get('name', 'Unknown'),
				'position': sensor_config.get('position', 'Unknown'),
				'pressure': sensor_config.get('pressure', 32.0),
				'temperature': sensor_config.get('temperature', 25.0),
				'battery_level': sensor_config.get('battery_level', 85),
				'last_update': datetime.now(),
				'low_threshold': sensor_config.get('low_threshold', 20),
				'high_threshold': sensor_config.get('high_threshold', 40),
				'target_pressure': sensor_config.get('target_pressure', 32.0),
				'pressure_range': sensor_config.get('pressure_range', [28.0, 36.0]),
				'temp_range': sensor_config.get('temp_range', [40.0, 120.0]),
				'status': 'normal'
			}

			logger.info(f"Added TPMS sensor: {sensor_id} at position {sensor_config.get('position', 'Unknown')}")

		except Exception as e:
			logger.error(f"Error adding TPMS sensor: {e}")

	def remove_sensor(self, sensor_id: str):
		"""Remove a TPMS sensor"""
		try:
			if sensor_id in self.sensor_data:
				del self.sensor_data[sensor_id]
				logger.info(f"Removed TPMS sensor: {sensor_id}")
			else:
				logger.warning(f"TPMS sensor {sensor_id} not found")

		except Exception as e:
			logger.error(f"Error removing TPMS sensor: {e}")

	def get_sensor_data(self, sensor_id: str) -> Optional[Dict[str, Any]]:
		"""Get data for a specific sensor"""
		return self.sensor_data.get(sensor_id)

	def get_all_sensors(self) -> Dict[str, Dict[str, Any]]:
		"""Get data for all sensors"""
		return self.sensor_data.copy()

	def get_sensor_status(self, sensor_id: str) -> Optional[str]:
		"""Get status for a specific sensor"""
		sensor = self.sensor_data.get(sensor_id)
		return sensor.get('status', 'unknown') if sensor else None

	def get_sensor_summary(self) -> Dict[str, Any]:
		"""Get summary of all sensors"""
		try:
			total_sensors = len(self.sensor_data)
			normal_sensors = len([s for s in self.sensor_data.values() if s.get('status') == 'normal'])
			alert_sensors = total_sensors - normal_sensors

			pressures = [s['pressure'] for s in self.sensor_data.values() if 'pressure' in s]
			temperatures = [s['temperature'] for s in self.sensor_data.values() if 'temperature' in s]
			batteries = [s['battery_level'] for s in self.sensor_data.values() if 'battery_level' in s]

			return {
				'total_sensors': total_sensors,
				'normal_sensors': normal_sensors,
				'alert_sensors': alert_sensors,
				'pressure_stats': {
					'min': min(pressures) if pressures else 0,
					'max': max(pressures) if pressures else 0,
					'avg': sum(pressures) / len(pressures) if pressures else 0
				},
				'temperature_stats': {
					'min': min(temperatures) if temperatures else 0,
					'max': max(temperatures) if temperatures else 0,
					'avg': sum(temperatures) / len(temperatures) if temperatures else 0
				},
				'battery_stats': {
					'min': min(batteries) if batteries else 0,
					'max': max(batteries) if batteries else 0,
					'avg': sum(batteries) / len(batteries) if batteries else 0
				}
			}

		except Exception as e:
			logger.error(f"Error getting sensor summary: {e}")
			return {}

	async def get_data(self) -> Dict[str, Any]:
		"""Get current TPMS data for API responses"""
		if not self.enabled:
			return {
				'enabled': False,
				'error': 'TPMS manager disabled'
			}

		return {
			'enabled': True,
			'sensors': self.sensor_data,
			'summary': self.get_sensor_summary(),
			'last_scan': self.last_scan_time.isoformat(),
			'timestamp': datetime.now().isoformat()
		}

	def cleanup(self):
		"""Cleanup hardware resources"""
		try:
			if self.spi:
				self.spi.close()
				self.spi = None

			logger.info("TPMS manager cleanup completed")

		except Exception as e:
			logger.error(f"Error during TPMS manager cleanup: {e}")
