"""
Environmental Sensor Manager for Sensor Hub
Monitors DHT22 sensors for cabin, outdoor, and refrigerator temperatures and humidity
"""

import asyncio
import time
import logging
from typing import Dict, List, Optional, Tuple, Any
from datetime import datetime, timedelta
from dataclasses import dataclass
import RPi.GPIO as GPIO

# Import DHT library for actual sensor reading
try:
	import Adafruit_DHT
	DHT_AVAILABLE = True
	logger = logging.getLogger(__name__)
	logger.info("Adafruit DHT library imported successfully")
except ImportError:
	DHT_AVAILABLE = False
	logger = logging.getLogger(__name__)
	logger.warning("Adafruit DHT library not available. Install with: pip install Adafruit_DHT")

@dataclass
class SensorReading:
	"""Represents a single sensor reading"""
	timestamp: datetime
	temperature: float  # Celsius
	humidity: float     # Percentage
	location: str
	sensor_id: str
	valid: bool = True

@dataclass
class SensorConfig:
	"""Configuration for a single sensor"""
	location: str
	pin: int
	sensor_type: str  # "DHT22" or "DHT11"
	update_interval: int  # milliseconds
	enabled: bool = True
	calibration_offset_temp: float = 0.0
	calibration_offset_humidity: float = 0.0

class EnvironmentalManager:
	def __init__(self, config: Dict):
		"""
		Initialize environmental sensor manager

		Args:
			config: Configuration dictionary with sensor settings
		"""
		self.config = config
		self.enabled = config.get('enabled', True)

		# Check DHT library availability
		if not DHT_AVAILABLE:
			raise ImportError(
				"Adafruit DHT library not available. "
				"Install with: pip install Adafruit_DHT"
			)

		# Sensor configurations
		self.sensors: Dict[str, SensorConfig] = {}
		self._init_sensors()

		# Sensor state
		self.readings: Dict[str, List[SensorReading]] = {}
		self.last_readings: Dict[str, SensorReading] = {}
		self.sensor_errors: Dict[str, int] = {}

		# Monitoring state
		self.monitoring = False
		self.monitor_task = None

		# Statistics
		self.stats = {
			'readings_total': 0,
			'readings_valid': 0,
			'readings_invalid': 0,
			'errors_total': 0,
			'last_update': None,
			'start_time': datetime.now()
		}

		# Alert thresholds
		self.alert_thresholds = {
			'temperature': {
				'cabin': {'min': 15.0, 'max': 35.0},      # 59°F - 95°F
				'outdoor': {'min': -20.0, 'max': 50.0},   # -4°F - 122°F
				'refrigerator': {'min': 2.0, 'max': 8.0}  # 36°F - 46°F
			},
			'humidity': {
				'cabin': {'min': 30.0, 'max': 70.0},      # 30% - 70%
				'outdoor': {'min': 10.0, 'max': 95.0},    # 10% - 95%
				'refrigerator': {'min': 20.0, 'max': 60.0} # 20% - 60%
			}
		}

		logger.info("Environmental manager initialized")

	def _init_sensors(self):
		"""Initialize sensor configurations from config"""
		try:
			sensor_configs = self.config.get('sensors', {})

			for sensor_id, sensor_config in sensor_configs.items():
				if sensor_config.get('enabled', True):
					self.sensors[sensor_id] = SensorConfig(
						location=sensor_config['location'],
						pin=sensor_config['pin'],
						sensor_type=sensor_config.get('sensor_type', 'DHT22'),
						update_interval=sensor_config.get('update_interval', 5000),
						calibration_offset_temp=sensor_config.get('calibration_offset_temp', 0.0),
						calibration_offset_humidity=sensor_config.get('calibration_offset_humidity', 0.0)
					)

					# Initialize readings storage
					self.readings[sensor_id] = []
					self.sensor_errors[sensor_id] = 0

					logger.info(f"Initialized sensor {sensor_id} at {sensor_config['location']} on pin {sensor_config['pin']}")

		except Exception as e:
			logger.error(f"Error initializing sensors: {e}")

	def _setup_gpio(self):
		"""Setup GPIO pins for sensors"""
		try:
			GPIO.setmode(GPIO.BCM)

			# Set all sensor pins as inputs with pull-up
			for sensor in self.sensors.values():
				GPIO.setup(sensor.pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)

			logger.info("GPIO setup completed for environmental sensors")

		except Exception as e:
			logger.error(f"Error setting up GPIO: {e}")
			raise

	async def start_monitoring(self):
		"""Start environmental monitoring"""
		if not self.enabled:
			logger.warning("Environmental monitoring is disabled")
			return

		if not self.sensors:
			logger.warning("No sensors configured for environmental monitoring")
			return

		self.monitoring = True
		self.monitor_task = asyncio.create_task(self._monitoring_loop())
		logger.info("Environmental monitoring started")

	async def stop_monitoring(self):
		"""Stop environmental monitoring"""
		self.monitoring = False

		if self.monitor_task:
			self.monitor_task.cancel()
			try:
				await self.monitor_task
			except asyncio.CancelledError:
				pass
			self.monitor_task = None

		logger.info("Environmental monitoring stopped")

	async def _monitoring_loop(self):
		"""Main monitoring loop for environmental sensors"""
		while self.monitoring:
			try:
				# Read all sensors
				await self._read_all_sensors()

				# Update statistics
				self._update_statistics()

				# Check for alerts
				await self._check_alerts()

				# Wait for next update cycle
				await asyncio.sleep(1)  # Check every second

			except asyncio.CancelledError:
				break
			except Exception as e:
				logger.error(f"Error in environmental monitoring loop: {e}")
				await asyncio.sleep(5)

	async def _read_all_sensors(self):
		"""Read all configured sensors"""
		for sensor_id, sensor_config in self.sensors.items():
			try:
				# Check if it's time to read this sensor
				if self._should_read_sensor(sensor_id, sensor_config):
					reading = await self._read_sensor(sensor_id, sensor_config)
					if reading:
						self._store_reading(sensor_id, reading)
						self.last_readings[sensor_id] = reading

			except Exception as e:
				logger.error(f"Error reading sensor {sensor_id}: {e}")
				self.sensor_errors[sensor_id] += 1

	def _should_read_sensor(self, sensor_id: str, sensor_config: SensorConfig) -> bool:
		"""Check if it's time to read a sensor"""
		if sensor_id not in self.last_readings:
			return True

		last_reading = self.last_readings[sensor_id]
		time_since_last = (datetime.now() - last_reading.timestamp).total_seconds()
		interval_seconds = sensor_config.update_interval / 1000.0

		return time_since_last >= interval_seconds

	async def _read_sensor(self, sensor_id: str, sensor_config: SensorConfig) -> Optional[SensorReading]:
		"""Read a single sensor"""
		try:
			# Determine sensor type
			if sensor_config.sensor_type.upper() == 'DHT22':
				sensor = Adafruit_DHT.DHT22
			elif sensor_config.sensor_type.upper() == 'DHT11':
				sensor = Adafruit_DHT.DHT11
			else:
				logger.error(f"Unknown sensor type: {sensor_config.sensor_type}")
				return None

			# Read sensor
			humidity, temperature = Adafruit_DHT.read_retry(sensor, sensor_config.pin)

			# Check if reading was successful
			if humidity is None or temperature is None:
				logger.warning(f"Failed to read sensor {sensor_id}")
				return None

			# Apply calibration offsets
			temperature += sensor_config.calibration_offset_temp
			humidity += sensor_config.calibration_offset_humidity

			# Validate readings
			if not self._validate_reading(temperature, humidity):
				logger.warning(f"Invalid reading from sensor {sensor_id}: T={temperature:.1f}°C, H={humidity:.1f}%")
				return SensorReading(
					timestamp=datetime.now(),
					temperature=temperature,
					humidity=humidity,
					location=sensor_config.location,
					sensor_id=sensor_id,
					valid=False
				)

			# Create reading object
			reading = SensorReading(
				timestamp=datetime.now(),
				temperature=temperature,
				humidity=humidity,
				location=sensor_config.location,
				sensor_id=sensor_id,
				valid=True
			)

			logger.debug(f"Read sensor {sensor_id}: {temperature:.1f}°C, {humidity:.1f}%")
			return reading

		except Exception as e:
			logger.error(f"Error reading sensor {sensor_id}: {e}")
			return None

	def _validate_reading(self, temperature: float, humidity: float) -> bool:
		"""Validate sensor reading values"""
		# Temperature range: -40°C to +80°C (typical DHT range)
		if temperature < -40.0 or temperature > 80.0:
			return False

		# Humidity range: 0% to 100%
		if humidity < 0.0 or humidity > 100.0:
			return False

		return True

	def _store_reading(self, sensor_id: str, reading: SensorReading):
		"""Store a sensor reading"""
		try:
			# Add to readings list
			self.readings[sensor_id].append(reading)

			# Keep only last 1000 readings per sensor
			if len(self.readings[sensor_id]) > 1000:
				self.readings[sensor_id] = self.readings[sensor_id][-1000:]

			# Update statistics
			self.stats['readings_total'] += 1
			if reading.valid:
				self.stats['readings_valid'] += 1
			else:
				self.stats['readings_invalid'] += 1

		except Exception as e:
			logger.error(f"Error storing reading for sensor {sensor_id}: {e}")

	def _update_statistics(self):
		"""Update monitoring statistics"""
		try:
			self.stats['last_update'] = datetime.now()

		except Exception as e:
			logger.error(f"Error updating statistics: {e}")

	async def _check_alerts(self):
		"""Check for environmental alerts"""
		try:
			for sensor_id, reading in self.last_readings.items():
				if not reading.valid:
					continue

				location = reading.location
				temperature = reading.temperature
				humidity = reading.humidity

				# Check temperature thresholds
				if location in self.alert_thresholds['temperature']:
					temp_thresholds = self.alert_thresholds['temperature'][location]
					if temperature < temp_thresholds['min']:
						logger.warning(f"Low temperature alert: {sensor_id} at {location} - {temperature:.1f}°C")
					elif temperature > temp_thresholds['max']:
						logger.warning(f"High temperature alert: {sensor_id} at {location} - {temperature:.1f}°C")

				# Check humidity thresholds
				if location in self.alert_thresholds['humidity']:
					humidity_thresholds = self.alert_thresholds['humidity'][location]
					if humidity < humidity_thresholds['min']:
						logger.warning(f"Low humidity alert: {sensor_id} at {location} - {humidity:.1f}%")
					elif humidity > humidity_thresholds['max']:
						logger.warning(f"High humidity alert: {sensor_id} at {location} - {humidity:.1f}%")

		except Exception as e:
			logger.error(f"Error checking alerts: {e}")

	async def update(self, timestamp: datetime):
		"""Update environmental manager (called by main loop)"""
		if not self.enabled:
			return

		try:
			# This method is called by the main loop
			# The actual monitoring happens in the background task
			pass

		except Exception as e:
			logger.error(f"Error updating environmental manager: {e}")

	def get_sensor_reading(self, sensor_id: str) -> Optional[SensorReading]:
		"""Get the most recent reading for a specific sensor"""
		return self.last_readings.get(sensor_id)

	def get_all_readings(self) -> Dict[str, SensorReading]:
		"""Get the most recent readings for all sensors"""
		return self.last_readings.copy()

	def get_sensor_history(self, sensor_id: str, limit: int = 100) -> List[SensorReading]:
		"""Get reading history for a specific sensor"""
		if sensor_id not in self.readings:
			return []

		readings = self.readings[sensor_id]
		return readings[-limit:] if len(readings) > limit else readings

	def get_location_readings(self, location: str) -> List[SensorReading]:
		"""Get all readings for a specific location"""
		location_readings = []

		for sensor_id, sensor_config in self.sensors.items():
			if sensor_config.location == location:
				location_readings.extend(self.readings.get(sensor_id, []))

		# Sort by timestamp
		location_readings.sort(key=lambda x: x.timestamp)
		return location_readings

	def get_temperature_stats(self, location: str, hours: int = 24) -> Dict[str, float]:
		"""Get temperature statistics for a location over time"""
		try:
			cutoff_time = datetime.now() - timedelta(hours=hours)
			readings = [r for r in self.get_location_readings(location)
					   if r.timestamp >= cutoff_time and r.valid]

			if not readings:
				return {'min': 0.0, 'max': 0.0, 'avg': 0.0, 'count': 0}

			temperatures = [r.temperature for r in readings]
			return {
				'min': min(temperatures),
				'max': max(temperatures),
				'avg': sum(temperatures) / len(temperatures),
				'count': len(readings)
			}

		except Exception as e:
			logger.error(f"Error calculating temperature stats for {location}: {e}")
			return {'min': 0.0, 'max': 0.0, 'avg': 0.0, 'count': 0}

	def get_humidity_stats(self, location: str, hours: int = 24) -> Dict[str, float]:
		"""Get humidity statistics for a location over time"""
		try:
			cutoff_time = datetime.now() - timedelta(hours=hours)
			readings = [r for r in self.get_location_readings(location)
					   if r.timestamp >= cutoff_time and r.valid]

			if not readings:
				return {'min': 0.0, 'max': 0.0, 'avg': 0.0, 'count': 0}

			humidities = [r.humidity for r in readings]
			return {
				'min': min(humidities),
				'max': max(humidities),
				'avg': sum(humidities) / len(humidities),
				'count': len(readings)
			}

		except Exception as e:
			logger.error(f"Error calculating humidity stats for {location}: {e}")
			return {'min': 0.0, 'max': 0.0, 'avg': 0.0, 'count': 0}

	def get_system_status(self) -> Dict[str, Any]:
		"""Get overall system status"""
		try:
			total_sensors = len(self.sensors)
			active_sensors = len([s for s in self.sensors.values() if s.enabled])
			sensors_with_data = len([s for s in self.sensors.keys() if s in self.last_readings])

			return {
				'total_sensors': total_sensors,
				'active_sensors': active_sensors,
				'sensors_with_data': sensors_with_data,
				'monitoring': self.monitoring,
				'last_update': self.stats['last_update'].isoformat() if self.stats['last_update'] else None,
				'uptime_seconds': (datetime.now() - self.stats['start_time']).total_seconds()
			}

		except Exception as e:
			logger.error(f"Error getting system status: {e}")
			return {}

	async def get_data(self) -> Dict[str, Any]:
		"""Get current environmental data for API responses"""
		if not self.enabled:
			return {
				'enabled': False,
				'error': 'Environmental manager disabled'
			}

		try:
			# Get current readings for all sensors
			current_readings = {}
			for sensor_id, sensor_config in self.sensors.items():
				if sensor_id in self.last_readings:
					reading = self.last_readings[sensor_id]
					current_readings[sensor_id] = {
						'temperature': reading.temperature,
						'humidity': reading.humidity,
						'location': reading.location,
						'timestamp': reading.timestamp.isoformat(),
						'valid': reading.valid
					}

			# Get statistics for each location
			location_stats = {}
			locations = set(sensor.location for sensor in self.sensors.values())
			for location in locations:
				location_stats[location] = {
					'temperature': self.get_temperature_stats(location),
					'humidity': self.get_humidity_stats(location)
				}

			return {
				'enabled': True,
				'sensors': current_readings,
				'location_stats': location_stats,
				'system_status': self.get_system_status(),
				'statistics': self.stats,
				'timestamp': datetime.now().isoformat()
			}

		except Exception as e:
			logger.error(f"Error getting environmental data: {e}")
			return {
				'enabled': True,
				'error': str(e),
				'timestamp': datetime.now().isoformat()
			}

	def add_sensor(self, sensor_config: Dict[str, Any]):
		"""Add a new environmental sensor"""
		try:
			sensor_id = sensor_config['id']
			if sensor_id in self.sensors:
				logger.warning(f"Environmental sensor {sensor_id} already exists")
				return

			self.sensors[sensor_id] = SensorConfig(
				location=sensor_config['location'],
				pin=sensor_config['pin'],
				sensor_type=sensor_config.get('sensor_type', 'DHT22'),
				update_interval=sensor_config.get('update_interval', 5000),
				calibration_offset_temp=sensor_config.get('calibration_offset_temp', 0.0),
				calibration_offset_humidity=sensor_config.get('calibration_offset_humidity', 0.0)
			)

			# Initialize storage
			self.readings[sensor_id] = []
			self.sensor_errors[sensor_id] = 0

			logger.info(f"Added environmental sensor: {sensor_id} at {sensor_config['location']}")

		except Exception as e:
			logger.error(f"Error adding environmental sensor: {e}")

	def remove_sensor(self, sensor_id: str):
		"""Remove an environmental sensor"""
		try:
			if sensor_id in self.sensors:
				del self.sensors[sensor_id]
				if sensor_id in self.readings:
					del self.readings[sensor_id]
				if sensor_id in self.last_readings:
					del self.last_readings[sensor_id]
				if sensor_id in self.sensor_errors:
					del self.sensor_errors[sensor_id]

				logger.info(f"Removed environmental sensor: {sensor_id}")
			else:
				logger.warning(f"Environmental sensor {sensor_id} not found")

		except Exception as e:
			logger.error(f"Error removing environmental sensor: {e}")

	def update_thresholds(self, location: str, temp_min: float = None, temp_max: float = None,
						 humidity_min: float = None, humidity_max: float = None):
		"""Update alert thresholds for a location"""
		try:
			if location not in self.alert_thresholds['temperature']:
				self.alert_thresholds['temperature'][location] = {'min': 0.0, 'max': 100.0}
			if location not in self.alert_thresholds['humidity']:
				self.alert_thresholds['humidity'][location] = {'min': 0.0, 'max': 100.0}

			if temp_min is not None:
				self.alert_thresholds['temperature'][location]['min'] = temp_min
			if temp_max is not None:
				self.alert_thresholds['temperature'][location]['max'] = temp_max
			if humidity_min is not None:
				self.alert_thresholds['humidity'][location]['min'] = humidity_min
			if humidity_max is not None:
				self.alert_thresholds['humidity'][location]['max'] = humidity_max

			logger.info(f"Updated thresholds for {location}")

		except Exception as e:
			logger.error(f"Error updating thresholds for {location}: {e}")

	def enable_sensor(self, location: str):
		"""Enable a specific sensor"""
		if location in self.sensors:
			self.sensors[location].enabled = True
			logger.info(f"Enabled {location} sensor")

	def disable_sensor(self, location: str):
		"""Disable a specific sensor"""
		if location in self.sensors:
			self.sensors[location].enabled = False
			logger.info(f"Disabled {location} sensor")

	def cleanup(self):
		"""Cleanup resources"""
		try:
			# Stop monitoring if running
			if self.monitoring:
				asyncio.create_task(self.stop_monitoring())

			# Cleanup GPIO
			GPIO.cleanup()

			logger.info("Environmental manager cleanup completed")

		except Exception as e:
			logger.error(f"Error during environmental manager cleanup: {e}")
