#!/usr/bin/env python3
"""
GPS Simulator for Mock Sensor Hub
Simulates GPS readings with configurable simulation modes and route tracking
"""

import random
import math
import logging
from datetime import datetime, timedelta
from typing import Dict, List, Optional, Any, Tuple

logger = logging.getLogger(__name__)

class GPSSimulator:
	def __init__(self, config: Dict):
		self.config = config
		self.enabled = config.get('enabled', True)
		self.simulation_mode = config.get('simulation_mode', 'static')
		self.update_interval = config.get('update_interval', 1.0)  # seconds

		# GPS state
		self.latitude = config.get('initial_latitude', 40.7128)  # NYC default
		self.longitude = config.get('initial_longitude', -74.0060)
		self.altitude = config.get('initial_altitude', 10.0)  # meters
		self.speed = config.get('initial_speed', 0.0)  # m/s
		self.heading = config.get('initial_heading', 0.0)  # degrees
		self.satellites = config.get('satellites', 8)
		self.hdop = config.get('hdop', 1.2)  # Horizontal dilution of precision
		self.fix_quality = config.get('fix_quality', 1)  # 0=invalid, 1=GPS, 2=DGPS

		# Route tracking
		self.current_route = None
		self.route_points = []
		self.route_start_time = None
		self.total_distance = 0.0
		self.max_speed = 0.0
		self.speed_history = []

		# Simulation parameters
		self.simulation_speed = config.get('simulation_speed', 1.0)  # multiplier
		self.altitude_variation = config.get('altitude_variation', 2.0)
		self.speed_variation = config.get('speed_variation', 0.5)
		self.heading_variation = config.get('heading_variation', 5.0)

		# Predefined routes for simulation
		self.predefined_routes = config.get('predefined_routes', [])
		self.current_route_index = 0

		# Engine state tracking
		self.engine_running = False
		self.last_position = (self.latitude, self.longitude)

	def start_route(self):
		"""Start tracking a new route"""
		if self.current_route is not None:
			logger.warning("Route already in progress, ending current route first")
			self.end_route()

		self.current_route = {
			'id': f"route_{datetime.now().strftime('%Y%m%d_%H%M%S')}",
			'start_time': datetime.now(),
			'start_position': (self.latitude, self.longitude),
			'points': []
		}

		self.route_start_time = datetime.now()
		self.route_points = []
		self.total_distance = 0.0
		self.max_speed = 0.0
		self.speed_history = []

		logger.info(f"Started new GPS route: {self.current_route['id']}")

	def end_route(self):
		"""End the current route and calculate statistics"""
		if self.current_route is None:
			logger.warning("No route in progress")
			return None

		# Add final point
		self._add_route_point()

		# Calculate route statistics
		end_time = datetime.now()
		duration = (end_time - self.route_start_time).total_seconds()
		avg_speed = self.total_distance / duration if duration > 0 else 0.0

		route_summary = {
			'id': self.current_route['id'],
			'start_time': self.route_start_time.isoformat(),
			'end_time': end_time.isoformat(),
			'duration_seconds': duration,
			'total_distance_meters': round(self.total_distance, 2),
			'max_speed_mps': round(self.max_speed, 2),
			'avg_speed_mps': round(avg_speed, 2),
			'total_points': len(self.route_points),
			'start_position': self.current_route['start_position'],
			'end_position': (self.latitude, self.longitude)
		}

		# Reset route state
		self.current_route = None
		self.route_start_time = None
		self.route_points = []

		logger.info(f"Ended GPS route: {route_summary['id']}")
		return route_summary

	def _add_route_point(self):
		"""Add current position to route tracking"""
		if self.current_route is None:
			return

		point = {
			'timestamp': datetime.now().isoformat(),
			'latitude': self.latitude,
			'longitude': self.longitude,
			'altitude': self.altitude,
			'speed': self.speed,
			'heading': self.heading,
			'satellites': self.satellites,
			'hdop': self.hdop
		}

		self.route_points.append(point)
		self.current_route['points'].append(point)

		# Update distance and speed tracking
		if len(self.route_points) > 1:
			prev_point = self.route_points[-2]
			distance = self._calculate_distance(
				prev_point['latitude'], prev_point['longitude'],
				self.latitude, self.longitude
			)
			self.total_distance += distance

		if self.speed > self.max_speed:
			self.max_speed = self.speed

		self.speed_history.append(self.speed)

	def _calculate_distance(self, lat1: float, lon1: float, lat2: float, lon2: float) -> float:
		"""Calculate distance between two GPS coordinates using Haversine formula"""
		R = 6371000  # Earth's radius in meters

		lat1_rad = math.radians(lat1)
		lat2_rad = math.radians(lat2)
		delta_lat = math.radians(lat2 - lat1)
		delta_lon = math.radians(lon2 - lon1)

		a = (math.sin(delta_lat / 2) ** 2 +
			 math.cos(lat1_rad) * math.cos(lat2_rad) * math.sin(delta_lon / 2) ** 2)
		c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))

		return R * c

	def _calculate_bearing(self, lat1: float, lon1: float, lat2: float, lon2: float) -> float:
		"""Calculate bearing between two GPS coordinates"""
		lat1_rad = math.radians(lat1)
		lat2_rad = math.radians(lat2)
		delta_lon = math.radians(lon2 - lon1)

		y = math.sin(delta_lon) * math.cos(lat2_rad)
		x = (math.cos(lat1_rad) * math.sin(lat2_rad) -
			 math.sin(lat1_rad) * math.cos(lat2_rad) * math.cos(delta_lon))

		bearing = math.degrees(math.atan2(y, x))
		return (bearing + 360) % 360

	def simulate_movement(self, engine_running: bool = None):
		"""Simulate GPS movement based on current state and simulation mode"""
		if engine_running is not None:
			self.engine_running = engine_running

		if not self.enabled:
			return

		if self.simulation_mode == 'static':
			self._simulate_static()
		elif self.simulation_mode == 'driving':
			self._simulate_driving()
		elif self.simulation_mode == 'route':
			self._simulate_route()
		else:
			self._simulate_static()

		# Add route point if tracking
		if self.current_route is not None:
			self._add_route_point()

	def _simulate_static(self):
		"""Simulate static GPS position with minor variations"""
		# Small random variations for realistic static positioning
		self.latitude += random.uniform(-0.00001, 0.00001)  # ~1 meter variation
		self.longitude += random.uniform(-0.00001, 0.00001)
		self.altitude += random.uniform(-self.altitude_variation, self.altitude_variation)
		self.speed = 0.0
		self.heading = random.uniform(-self.heading_variation, self.heading_variation)

		# Simulate satellite variations
		self.satellites = max(4, min(12, self.satellites + random.randint(-1, 1)))
		self.hdop = max(0.8, min(3.0, self.hdop + random.uniform(-0.1, 0.1)))

	def _simulate_driving(self):
		"""Simulate driving GPS movement"""
		if not self.engine_running:
			self._simulate_static()
			return

		# Simulate realistic driving movement
		speed_change = random.uniform(-self.speed_variation, self.speed_variation)
		self.speed = max(0, min(30, self.speed + speed_change))  # 0-30 m/s (0-67 mph)

		if self.speed > 0:
			# Calculate new position based on speed and heading
			distance = self.speed * self.update_interval
			lat_change = distance * math.cos(math.radians(self.heading)) / 111000  # Approx meters to degrees
			lon_change = distance * math.sin(math.radians(self.heading)) / (111000 * math.cos(math.radians(self.latitude)))

			self.latitude += lat_change
			self.longitude += lon_change

			# Simulate heading changes (gentle curves)
			heading_change = random.uniform(-self.heading_variation, self.heading_variation)
			self.heading = (self.heading + heading_change) % 360

			# Simulate altitude changes (hills)
			altitude_change = random.uniform(-self.altitude_variation, self.altitude_variation)
			self.altitude = max(0, self.altitude + altitude_change)

		# Update satellite and precision data
		self.satellites = max(6, min(12, self.satellites + random.randint(-1, 1)))
		self.hdop = max(0.8, min(2.5, self.hdop + random.uniform(-0.1, 0.1)))

	def _simulate_route(self):
		"""Simulate GPS movement along predefined routes"""
		if not self.predefined_routes:
			self._simulate_driving()
			return

		# Get current route waypoints
		route = self.predefined_routes[self.current_route_index]
		waypoints = route.get('waypoints', [])

		if not waypoints:
			self._simulate_driving()
			return

		# Find nearest waypoint
		current_pos = (self.latitude, self.longitude)
		nearest_idx = 0
		nearest_distance = float('inf')

		for i, waypoint in enumerate(waypoints):
			distance = self._calculate_distance(
				current_pos[0], current_pos[1],
				waypoint['latitude'], waypoint['longitude']
			)
			if distance < nearest_distance:
				nearest_distance = distance
				nearest_idx = i

		# Move toward next waypoint
		next_idx = (nearest_idx + 1) % len(waypoints)
		next_waypoint = waypoints[next_idx]

		# Calculate bearing to next waypoint
		target_bearing = self._calculate_bearing(
			self.latitude, self.longitude,
			next_waypoint['latitude'], next_waypoint['longitude']
		)

		# Gradually adjust heading toward target
		heading_diff = (target_bearing - self.heading + 180) % 360 - 180
		heading_adjustment = max(-self.heading_variation, min(self.heading_variation, heading_diff))
		self.heading = (self.heading + heading_adjustment) % 360

		# Move forward
		if self.engine_running:
			self.speed = max(5, min(25, self.speed + random.uniform(-1, 1)))
			distance = self.speed * self.update_interval

			lat_change = distance * math.cos(math.radians(self.heading)) / 111000
			lon_change = distance * math.sin(math.radians(self.heading)) / (111000 * math.cos(math.radians(self.latitude)))

			self.latitude += lat_change
			self.longitude += lon_change

			# Update altitude based on waypoint
			target_altitude = next_waypoint.get('altitude', self.altitude)
			altitude_diff = target_altitude - self.altitude
			altitude_adjustment = max(-self.altitude_variation, min(self.altitude_variation, altitude_diff))
			self.altitude += altitude_adjustment

		# Check if we've reached the waypoint
		distance_to_waypoint = self._calculate_distance(
			self.latitude, self.longitude,
			next_waypoint['latitude'], next_waypoint['longitude']
		)

		if distance_to_waypoint < 10:  # Within 10 meters
			self.current_route_index = (self.current_route_index + 1) % len(self.predefined_routes)

	def get_current_position(self) -> Dict:
		"""Get current GPS position and status"""
		return {
			'latitude': round(self.latitude, 6),
			'longitude': round(self.longitude, 6),
			'altitude': round(self.altitude, 1),
			'speed': round(self.speed, 2),
			'heading': round(self.heading, 1),
			'satellites': self.satellites,
			'hdop': round(self.hdop, 2),
			'fix_quality': self.fix_quality,
			'timestamp': datetime.now().isoformat(),
			'engine_running': self.engine_running
		}

	def get_route_summary(self) -> Optional[Dict]:
		"""Get summary of current route if active"""
		if self.current_route is None:
			return None

		return {
			'route_id': self.current_route['id'],
			'start_time': self.route_start_time.isoformat(),
			'current_position': (self.latitude, self.longitude),
			'total_distance': round(self.total_distance, 2),
			'max_speed': round(self.max_speed, 2),
			'points_count': len(self.route_points),
			'duration_seconds': (datetime.now() - self.route_start_time).total_seconds()
		}

	def get_speed_history(self, max_points: int = 100) -> List[float]:
		"""Get recent speed history"""
		return self.speed_history[-max_points:] if self.speed_history else []

	def set_position(self, latitude: float, longitude: float, altitude: float = None):
		"""Manually set GPS position"""
		self.latitude = latitude
		self.longitude = longitude
		if altitude is not None:
			self.altitude = altitude

		logger.info(f"GPS position manually set to: {latitude}, {longitude}")

	def set_speed(self, speed: float):
		"""Manually set GPS speed"""
		self.speed = max(0, speed)
		logger.info(f"GPS speed manually set to: {self.speed} m/s")

	def set_heading(self, heading: float):
		"""Manually set GPS heading"""
		self.heading = heading % 360
		logger.info(f"GPS heading manually set to: {self.heading}°")

	def update_satellite_info(self, satellites: int, hdop: float, fix_quality: int):
		"""Update satellite and precision information"""
		self.satellites = max(0, min(12, satellites))
		self.hdop = max(0.8, min(10.0, hdop))
		self.fix_quality = max(0, min(2, fix_quality))

		logger.debug(f"Updated satellite info: {satellites} satellites, HDOP: {hdop}, Quality: {fix_quality}")

	async def update(self, current_time: datetime = None):
		"""Update GPS simulator (called by main simulation loop)"""
		if not self.enabled:
			return

		# Simulate movement
		self.simulate_movement()

		# Log significant position changes
		current_pos = (self.latitude, self.longitude)
		distance_moved = self._calculate_distance(
			self.last_position[0], self.last_position[1],
			current_pos[0], current_pos[1]
		)

		if distance_moved > 100:  # Log if moved more than 100 meters
			logger.info(f"GPS moved {distance_moved:.1f}m to {current_pos[0]:.6f}, {current_pos[1]:.6f}")
			self.last_position = current_pos

	def get_data(self) -> Dict:
		"""Get current GPS data for API responses"""
		if not self.enabled:
			return {
				'enabled': False,
				'error': 'GPS simulator disabled'
			}

		return {
			'enabled': True,
			'position': self.get_current_position(),
			'route_summary': self.get_route_summary(),
			'simulation_mode': self.simulation_mode,
			'engine_running': self.engine_running
		}
