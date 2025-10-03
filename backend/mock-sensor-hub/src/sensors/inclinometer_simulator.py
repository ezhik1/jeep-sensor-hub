#!/usr/bin/env python3
"""
Inclinometer Simulator for Mock Sensor Hub
Simulates MPU6050 inclinometer readings with realistic pitch, roll, and acceleration data
"""

import random
import math
import logging
from datetime import datetime, timedelta
from typing import Dict, List, Optional, Any, Tuple

logger = logging.getLogger(__name__)

class InclinometerSimulator:
	def __init__(self, config: Dict):
		self.config = config
		self.enabled = config.get('enabled', True)
		self.update_interval = config.get('update_interval', 0.1)  # seconds (10 Hz)

		# MPU6050 configuration
		self.accelerometer_range = config.get('accelerometer_range', 2)  # ±2g, ±4g, ±8g, ±16g
		self.gyroscope_range = config.get('gyroscope_range', 250)  # ±250, ±500, ±1000, ±2000 °/s
		self.sample_rate = config.get('sample_rate', 1000)  # Hz

		# Sensor state
		self.accel_x = 0.0  # g
		self.accel_y = 0.0  # g
		self.accel_z = 1.0  # g (gravity)
		self.gyro_x = 0.0   # °/s
		self.gyro_y = 0.0   # °/s
		self.gyro_z = 0.0   # °/s

		# Calculated values
		self.pitch = 0.0     # degrees
		self.roll = 0.0      # degrees
		self.yaw = 0.0       # degrees
		self.temperature = 25.0  # °C

		# Movement simulation
		self.movement_mode = config.get('movement_mode', 'static')  # static, gentle, moderate, aggressive
		self.vehicle_speed = 0.0  # m/s
		self.road_conditions = config.get('road_conditions', 'smooth')  # smooth, rough, offroad
		self.terrain_type = config.get('terrain_type', 'flat')  # flat, hilly, mountainous

		# Calibration and filtering
		self.accel_offset = config.get('accel_offset', {'x': 0.0, 'y': 0.0, 'z': 0.0})
		self.gyro_offset = config.get('gyro_offset', {'x': 0.0, 'y': 0.0, 'z': 0.0})
		self.noise_level = config.get('noise_level', 0.02)  # g
		self.drift_rate = config.get('drift_rate', 0.001)  # °/s

		# History for filtering
		self.accel_history = []
		self.gyro_history = []
		self.filter_window = config.get('filter_window', 10)

		# Vehicle dynamics
		self.vehicle_orientation = {
			'pitch': 0.0,  # degrees
			'roll': 0.0,   # degrees
			'yaw': 0.0     # degrees
		}

		# Road simulation
		self.road_segments = config.get('road_segments', [])
		self.current_segment = 0
		self.segment_progress = 0.0

	def update_vehicle_state(self, speed: float = None,
		road_conditions: str = None,
		terrain_type: str = None,
		movement_mode: str = None):
		"""Update vehicle state for simulation"""
		if speed is not None:
			self.vehicle_speed = speed

		if road_conditions is not None:
			self.road_conditions = road_conditions

		if terrain_type is not None:
			self.terrain_type = terrain_type

		if movement_mode is not None:
			self.movement_mode = movement_mode

		# Update movement parameters based on new state
		self._update_movement_parameters()

	def _update_movement_parameters(self):
		"""Update movement simulation parameters based on current state"""
		# Adjust noise and drift based on road conditions
		if self.road_conditions == 'smooth':
			self.noise_level = 0.01
			self.drift_rate = 0.0005
		elif self.road_conditions == 'rough':
			self.noise_level = 0.05
			self.drift_rate = 0.002
		elif self.road_conditions == 'offroad':
			self.noise_level = 0.1
			self.drift_rate = 0.005

		# Adjust movement intensity based on terrain
		if self.terrain_type == 'flat':
			self.movement_mode = 'gentle'
		elif self.terrain_type == 'hilly':
			self.movement_mode = 'moderate'
		elif self.terrain_type == 'mountainous':
			self.movement_mode = 'aggressive'

	def simulate_movement(self):
		"""Simulate vehicle movement and update sensor readings"""
		if self.movement_mode == 'static':
			self._simulate_static_movement()
		elif self.movement_mode == 'gentle':
			self._simulate_gentle_movement()
		elif self.movement_mode == 'moderate':
			self._simulate_moderate_movement()
		elif self.movement_mode == 'aggressive':
			self._simulate_aggressive_movement()

		# Update calculated values
		self._calculate_orientation()

		# Apply filtering
		self._apply_sensor_filtering()

	def _simulate_static_movement(self):
		"""Simulate static vehicle with minimal movement"""
		# Small random variations for realistic static positioning
		self.accel_x += random.uniform(-0.01, 0.01)
		self.accel_y += random.uniform(-0.01, 0.01)
		self.accel_z = 1.0 + random.uniform(-0.02, 0.02)  # Gravity with small variation

		# Minimal gyroscope movement
		self.gyro_x += random.uniform(-0.1, 0.1)
		self.gyro_y += random.uniform(-0.1, 0.1)
		self.gyro_z += random.uniform(-0.1, 0.1)

	def _simulate_gentle_movement(self):
		"""Simulate gentle vehicle movement (smooth roads, low speed)"""
		# Gentle acceleration variations
		accel_variation = 0.05
		self.accel_x += random.uniform(-accel_variation, accel_variation)
		self.accel_y += random.uniform(-accel_variation, accel_variation)
		self.accel_z = 1.0 + random.uniform(-0.1, 0.1)

		# Gentle rotation
		gyro_variation = 0.5
		self.gyro_x += random.uniform(-gyro_variation, gyro_variation)
		self.gyro_y += random.uniform(-gyro_variation, gyro_variation)
		self.gyro_z += random.uniform(-gyro_variation, gyro_variation)

	def _simulate_moderate_movement(self):
		"""Simulate moderate vehicle movement (hills, moderate speed)"""
		# Moderate acceleration variations
		accel_variation = 0.15
		self.accel_x += random.uniform(-accel_variation, accel_variation)
		self.accel_y += random.uniform(-accel_variation, accel_variation)
		self.accel_z = 1.0 + random.uniform(-0.2, 0.2)

		# Moderate rotation
		gyro_variation = 2.0
		self.gyro_x += random.uniform(-gyro_variation, gyro_variation)
		self.gyro_y += random.uniform(-gyro_variation, gyro_variation)
		self.gyro_z += random.uniform(-gyro_variation, gyro_variation)

	def _simulate_aggressive_movement(self):
		"""Simulate aggressive vehicle movement (off-road, high speed)"""
		# Aggressive acceleration variations
		accel_variation = 0.3
		self.accel_x += random.uniform(-accel_variation, accel_variation)
		self.accel_y += random.uniform(-accel_variation, accel_variation)
		self.accel_z = 1.0 + random.uniform(-0.4, 0.4)

		# Aggressive rotation
		gyro_variation = 5.0
		self.gyro_x += random.uniform(-gyro_variation, gyro_variation)
		self.gyro_y += random.uniform(-gyro_variation, gyro_variation)
		self.gyro_z += random.uniform(-gyro_variation, gyro_variation)

	def _calculate_orientation(self):
		"""Calculate pitch, roll, and yaw from accelerometer and gyroscope data"""
		# Calculate pitch and roll from accelerometer
		# Pitch: rotation around X-axis (forward/backward tilt)
		self.pitch = math.degrees(math.atan2(-self.accel_x, math.sqrt(self.accel_y**2 + self.accel_z**2)))

		# Roll: rotation around Y-axis (left/right tilt)
		self.roll = math.degrees(math.atan2(self.accel_y, self.accel_z))

		# Yaw: rotation around Z-axis (compass heading)
		# Note: Yaw cannot be determined from accelerometer alone
		# This is a simplified calculation using gyroscope integration
		self.yaw += self.gyro_z * self.update_interval
		self.yaw = self.yaw % 360  # Keep in 0-360 range

	def _apply_sensor_filtering(self):
		"""Apply low-pass filtering to reduce noise"""
		# Add current readings to history
		current_accel = {'x': self.accel_x, 'y': self.accel_y, 'z': self.accel_z}
		current_gyro = {'x': self.gyro_x, 'y': self.gyro_y, 'z': self.gyro_z}

		self.accel_history.append(current_accel)
		self.gyro_history.append(current_gyro)

		# Keep only the last N readings
		if len(self.accel_history) > self.filter_window:
			self.accel_history.pop(0)
		if len(self.gyro_history) > self.filter_window:
			self.gyro_history.pop(0)

		# Apply moving average filter if we have enough data
		if len(self.accel_history) >= 3:
			self._apply_moving_average()

	def _apply_moving_average(self):
		"""Apply moving average filter to smooth sensor readings"""
		# Calculate average for accelerometer
		avg_accel_x = sum(reading['x'] for reading in self.accel_history) / len(self.accel_history)
		avg_accel_y = sum(reading['y'] for reading in self.accel_history) / len(self.accel_history)
		avg_accel_z = sum(reading['z'] for reading in self.accel_history) / len(self.accel_history)

		# Calculate average for gyroscope
		avg_gyro_x = sum(reading['x'] for reading in self.gyro_history) / len(self.gyro_history)
		avg_gyro_y = sum(reading['y'] for reading in self.gyro_history) / len(self.gyro_history)
		avg_gyro_z = sum(reading['z'] for reading in self.gyro_history) / len(self.gyro_history)

		# Apply weighted average (more weight to current reading)
		weight_current = 0.7
		weight_history = 1.0 - weight_current

		self.accel_x = (self.accel_x * weight_current + avg_accel_x * weight_history)
		self.accel_y = (self.accel_y * weight_current + avg_accel_y * weight_history)
		self.accel_z = (self.accel_z * weight_current + avg_accel_z * weight_history)

		self.gyro_x = (self.gyro_x * weight_current + avg_gyro_x * weight_history)
		self.gyro_y = (self.gyro_y * weight_current + avg_gyro_y * weight_history)
		self.gyro_z = (self.gyro_z * weight_current + avg_gyro_z * weight_history)

	def add_sensor_noise(self):
		"""Add realistic sensor noise to readings"""
		# Accelerometer noise
		self.accel_x += random.uniform(-self.noise_level, self.noise_level)
		self.accel_y += random.uniform(-self.noise_level, self.noise_level)
		self.accel_z += random.uniform(-self.noise_level, self.noise_level)

		# Gyroscope noise
		gyro_noise = self.drift_rate * self.update_interval
		self.gyro_x += random.uniform(-gyro_noise, gyro_noise)
		self.gyro_y += random.uniform(-gyro_noise, gyro_noise)
		self.gyro_z += random.uniform(-gyro_noise, gyro_noise)

	def get_current_reading(self) -> Dict:
		"""Get current inclinometer reading"""
		return {
			'accelerometer': {
				'x': round(self.accel_x, 4),  # g
				'y': round(self.accel_y, 4),  # g
				'z': round(self.accel_z, 4)   # g
			},
			'gyroscope': {
				'x': round(self.gyro_x, 2),   # °/s
				'y': round(self.gyro_y, 2),   # °/s
				'z': round(self.gyro_z, 2)    # °/s
			},
			'orientation': {
				'pitch': round(self.pitch, 2),  # degrees
				'roll': round(self.roll, 2),    # degrees
				'yaw': round(self.yaw, 2)       # degrees
			},
			'temperature': round(self.temperature, 1),  # °C
			'timestamp': datetime.now().isoformat()
		}

	def get_sensor_status(self) -> Dict:
		"""Get sensor status and health information"""
		# Check if readings are within expected ranges
		accel_magnitude = math.sqrt(self.accel_x**2 + self.accel_y**2 + self.accel_z**2)
		expected_gravity = 1.0  # 1g
		gravity_tolerance = 0.2

		status = 'normal'
		if abs(accel_magnitude - expected_gravity) > gravity_tolerance:
			status = 'warning'

		# Check for excessive movement
		gyro_magnitude = math.sqrt(self.gyro_x**2 + self.gyro_y**2 + self.gyro_z**2)
		if gyro_magnitude > 50:  # More than 50°/s
			status = 'high_movement'

		return {
			'status': status,
			'accel_magnitude': round(accel_magnitude, 3),
			'gyro_magnitude': round(gyro_magnitude, 2),
			'filter_window_size': len(self.accel_history),
			'movement_mode': self.movement_mode,
			'road_conditions': self.road_conditions,
			'terrain_type': self.terrain_type
		}

	def calibrate_sensor(self):
		"""Calibrate sensor by setting current readings as zero offsets"""
		self.accel_offset['x'] = -self.accel_x
		self.accel_offset['y'] = -self.accel_y
		self.accel_offset['z'] = -(self.accel_z - 1.0)  # Adjust for gravity

		self.gyro_offset['x'] = -self.gyro_x
		self.gyro_offset['y'] = -self.gyro_y
		self.gyro_offset['z'] = -self.gyro_z

		logger.info("Inclinometer sensor calibrated")

	def reset_orientation(self):
		"""Reset orientation angles to zero"""
		self.pitch = 0.0
		self.roll = 0.0
		self.yaw = 0.0
		logger.info("Orientation reset to zero")

	async def update(self, current_time: datetime = None):
		"""Update inclinometer simulator (called by main simulation loop)"""
		if not self.enabled:
			return

		# Simulate movement
		self.simulate_movement()

		# Add sensor noise
		self.add_sensor_noise()

		# Update temperature (small variations)
		self.temperature += random.uniform(-0.1, 0.1)
		self.temperature = max(-40, min(85, self.temperature))  # MPU6050 operating range

	def get_data(self) -> Dict:
		"""Get current inclinometer data for API responses"""
		if not self.enabled:
			return {
				'enabled': False,
				'error': 'Inclinometer simulator disabled'
			}

		return {
			'enabled': True,
			'reading': self.get_current_reading(),
			'status': self.get_sensor_status(),
			'configuration': {
				'accelerometer_range': f"±{self.accelerometer_range}g",
				'gyroscope_range': f"±{self.gyroscope_range}°/s",
				'sample_rate': f"{self.sample_rate} Hz",
				'filter_window': self.filter_window
			}
		}
