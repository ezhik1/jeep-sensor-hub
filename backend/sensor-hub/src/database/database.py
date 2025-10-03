#!/usr/bin/env python3
"""
Database Manager for Sensor Hub
Handles SQLite database operations for sensor data storage and retrieval
"""

import asyncio
import aiosqlite
import json
import logging
from datetime import datetime, timedelta
from typing import Dict, List, Optional, Any
from pathlib import Path

logger = logging.getLogger(__name__)

class DatabaseManager:
	def __init__(self, db_path: str):
		self.db_path = db_path
		self.db = None
		self._init_lock = asyncio.Lock()

	async def initialize(self):
		"""Initialize database connection and create tables"""
		async with self._init_lock:
			if self.db is None:
				self.db = await aiosqlite.connect(self.db_path)
				await self.create_tables()
				logger.info(f"Database initialized: {self.db_path}")

	async def create_tables(self):
		"""Create database tables if they don't exist"""
		async with self.db.execute("""
			CREATE TABLE IF NOT EXISTS sensor_readings (
				id INTEGER PRIMARY KEY AUTOINCREMENT,
				sensor_type TEXT NOT NULL,
				sensor_id TEXT,
				value REAL,
				unit TEXT,
				timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
				metadata TEXT
			)
		"""):
			pass

		async with self.db.execute("""
			CREATE TABLE IF NOT EXISTS gps_routes (
				id INTEGER PRIMARY KEY AUTOINCREMENT,
				route_id TEXT UNIQUE NOT NULL,
				start_time DATETIME NOT NULL,
				end_time DATETIME,
				total_distance REAL,
				max_speed REAL,
				avg_speed REAL,
				waypoints TEXT,
				metadata TEXT
			)
		"""):
			pass

		async with self.db.execute("""
			CREATE TABLE IF NOT EXISTS gps_waypoints (
				id INTEGER PRIMARY KEY AUTOINCREMENT,
				route_id TEXT NOT NULL,
				latitude REAL NOT NULL,
				longitude REAL NOT NULL,
				altitude REAL,
				speed REAL,
				timestamp DATETIME NOT NULL,
				engine_state BOOLEAN,
				FOREIGN KEY (route_id) REFERENCES gps_routes (route_id)
			)
		"""):
			pass

		async with self.db.execute("""
			CREATE TABLE IF NOT EXISTS engine_events (
				id INTEGER PRIMARY KEY AUTOINCREMENT,
				event_type TEXT NOT NULL,
				timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
				metadata TEXT
			)
		"""):
			pass

		async with self.db.execute("""
			CREATE TABLE IF NOT EXISTS cooling_history (
				id INTEGER PRIMARY KEY AUTOINCREMENT,
				temperature REAL NOT NULL,
				fan_low_speed BOOLEAN,
				fan_high_speed BOOLEAN,
				ac_state BOOLEAN,
				engine_load REAL,
				ambient_temp REAL,
				timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
			)
		"""):
			pass

		await self.db.commit()

	async def store_sensor_reading(self, sensor_type: str, sensor_id: str, value: float, unit: str, metadata: Dict = None):
		"""Store a single sensor reading"""
		try:
			metadata_json = json.dumps(metadata) if metadata else None
			async with self.db.execute("""
				INSERT INTO sensor_readings (sensor_type, sensor_id, value, unit, metadata)
				VALUES (?, ?, ?, ?, ?)
			""", (sensor_type, sensor_id, value, unit, metadata_json)):
				pass
			await self.db.commit()
			logger.debug(f"Stored {sensor_type} reading: {value} {unit}")

		except Exception as e:
			logger.error(f"Error storing sensor reading: {e}")

	async def store_route_point(self, route_id: str, latitude: float, longitude: float, altitude: float = None, speed: float = None, engine_state: bool = None):
		"""Store a GPS route waypoint"""
		try:
			async with self.db.execute("""
				INSERT INTO gps_waypoints (route_id, latitude, longitude, altitude, speed, timestamp, engine_state)
				VALUES (?, ?, ?, ?, ?, ?, ?)
			""", (route_id, latitude, longitude, altitude, speed, datetime.now(), engine_state)):
				pass
			await self.db.commit()

		except Exception as e:
			logger.error(f"Error storing route point: {e}")

	async def create_route(self, route_id: str, start_time: datetime, metadata: Dict = None):
		"""Create a new GPS route"""
		try:
			metadata_json = json.dumps(metadata) if metadata else None
			async with self.db.execute("""
				INSERT INTO gps_routes (route_id, start_time, metadata)
				VALUES (?, ?, ?)
			""", (route_id, start_time, metadata_json)):
				pass
			await self.db.commit()
			logger.info(f"Created new route: {route_id}")

		except Exception as e:
			logger.error(f"Error creating route: {e}")

	async def update_route(self, route_id: str, end_time: datetime = None, total_distance: float = None, max_speed: float = None, avg_speed: float = None):
		"""Update route information"""
		try:
			updates = []
			values = []

			if end_time is not None:
				updates.append("end_time = ?")
				values.append(end_time)
			if total_distance is not None:
				updates.append("total_distance = ?")
				values.append(total_distance)
			if max_speed is not None:
				updates.append("max_speed = ?")
				values.append(max_speed)
			if avg_speed is not None:
				updates.append("avg_speed = ?")
				values.append(avg_speed)

			if updates:
				values.append(route_id)
				query = f"UPDATE gps_routes SET {', '.join(updates)} WHERE route_id = ?"
				async with self.db.execute(query, values):
					pass
				await self.db.commit()

		except Exception as e:
			logger.error(f"Error updating route: {e}")

	async def store_engine_event(self, event_type: str, metadata: Dict = None):
		"""Store an engine event"""
		try:
			metadata_json = json.dumps(metadata) if metadata else None
			async with self.db.execute("""
				INSERT INTO engine_events (event_type, metadata)
				VALUES (?, ?)
			""", (event_type, metadata_json)):
				pass
			await self.db.commit()

		except Exception as e:
			logger.error(f"Error storing engine event: {e}")

	async def store_cooling_data(self, temperature: float, fan_low_speed: bool, fan_high_speed: bool, ac_state: bool, engine_load: float, ambient_temp: float):
		"""Store cooling system data"""
		try:
			async with self.db.execute("""
				INSERT INTO cooling_history (temperature, fan_low_speed, fan_high_speed, ac_state, engine_load, ambient_temp)
				VALUES (?, ?, ?, ?, ?, ?)
			""", (temperature, fan_low_speed, fan_high_speed, ac_state, engine_load, ambient_temp)):
				pass
			await self.db.commit()

		except Exception as e:
			logger.error(f"Error storing cooling data: {e}")

	async def get_sensor_history(self, sensor_type: str, hours: int = 24) -> List[Dict]:
		"""Get sensor history for a specific sensor type"""
		try:
			cutoff_time = datetime.now() - timedelta(hours=hours)
			async with self.db.execute("""
				SELECT sensor_id, value, unit, timestamp, metadata
				FROM sensor_readings
				WHERE sensor_type = ? AND timestamp >= ?
				ORDER BY timestamp DESC
			""", (sensor_type, cutoff_time)) as cursor:
				rows = await cursor.fetchall()
				return [
					{
						'sensor_id': row[0],
						'value': row[1],
						'unit': row[2],
						'timestamp': row[3],
						'metadata': json.loads(row[4]) if row[4] else None
					}
					for row in rows
				]

		except Exception as e:
			logger.error(f"Error retrieving sensor history: {e}")
			return []

	async def get_routes(self, limit: int = 100) -> List[Dict]:
		"""Get recent GPS routes"""
		try:
			async with self.db.execute("""
				SELECT route_id, start_time, end_time, total_distance, max_speed, avg_speed, metadata
				FROM gps_routes
				ORDER BY start_time DESC
				LIMIT ?
			""", (limit,)) as cursor:
				rows = await cursor.fetchall()
				return [
					{
						'route_id': row[0],
						'start_time': row[1],
						'end_time': row[2],
						'total_distance': row[3],
						'max_speed': row[4],
						'avg_speed': row[5],
						'metadata': json.loads(row[6]) if row[6] else None
					}
					for row in rows
				]

		except Exception as e:
			logger.error(f"Error retrieving routes: {e}")
			return []

	async def get_route_waypoints(self, route_id: str) -> List[Dict]:
		"""Get waypoints for a specific route"""
		try:
			async with self.db.execute("""
				SELECT latitude, longitude, altitude, speed, timestamp, engine_state
				FROM gps_waypoints
				WHERE route_id = ?
				ORDER BY timestamp ASC
			""", (route_id,)) as cursor:
				rows = await cursor.fetchall()
				return [
					{
						'latitude': row[0],
						'longitude': row[1],
						'altitude': row[2],
						'speed': row[3],
						'timestamp': row[4],
						'engine_state': row[5]
					}
					for row in rows
				]

		except Exception as e:
			logger.error(f"Error retrieving route waypoints: {e}")
			return []

	async def export_data(self, sensor_type: str, hours: int = 24, format: str = "json") -> str:
		"""Export sensor data in specified format"""
		try:
			data = await self.get_sensor_history(sensor_type, hours)

			if format.lower() == "csv":
				return self._export_csv(data)
			else:
				return self._export_json(data)

		except Exception as e:
			logger.error(f"Error exporting data: {e}")
			return ""

	def _export_csv(self, data: List[Dict]) -> str:
		"""Export data as CSV"""
		if not data:
			return ""

		import csv
		import io

		output = io.StringIO()
		writer = csv.writer(output)

		# Write header
		writer.writerow(['sensor_id', 'value', 'unit', 'timestamp', 'metadata'])

		# Write data
		for row in data:
			metadata_str = json.dumps(row['metadata']) if row['metadata'] else ""
			writer.writerow([
				row['sensor_id'],
				row['value'],
				row['unit'],
				row['timestamp'],
				metadata_str
			])

		return output.getvalue()

	def _export_json(self, data: List[Dict]) -> str:
		"""Export data as JSON"""
		return json.dumps(data, indent=2, default=str)

	async def clear_data(self, sensor_type: str = None, days: int = None):
		"""Clear old data from database"""
		try:
			if days:
				cutoff_time = datetime.now() - timedelta(days=days)
				if sensor_type:
					async with self.db.execute("""
						DELETE FROM sensor_readings
						WHERE sensor_type = ? AND timestamp < ?
					""", (sensor_type, cutoff_time)):
						pass
				else:
					async with self.db.execute("""
						DELETE FROM sensor_readings
						WHERE timestamp < ?
					""", (cutoff_time,)):
						pass
			else:
				if sensor_type:
					async with self.db.execute("DELETE FROM sensor_readings WHERE sensor_type = ?", (sensor_type,)):
						pass
				else:
					async with self.db.execute("DELETE FROM sensor_readings"):
						pass

			await self.db.commit()
			logger.info(f"Cleared data for {sensor_type or 'all sensors'}")

		except Exception as e:
			logger.error(f"Error clearing data: {e}")

	async def store_sensor_data(self, data: Dict):
		"""Store sensor data from main sensor hub"""
		try:
			timestamp = data.get('timestamp', datetime.now())

			# Store TPMS data
			if data.get('tpms'):
				tpms_data = data['tpms']
				for sensor_id, sensor_info in tpms_data.get('sensors', {}).items():
					if 'pressure' in sensor_info:
						await self.store_sensor_reading(
							'tpms_pressure', sensor_id,
							sensor_info['pressure'], 'PSI',
							{'temperature': sensor_info.get('temperature'), 'battery': sensor_info.get('battery')}
						)
					if 'temperature' in sensor_info:
						await self.store_sensor_reading(
							'tpms_temperature', sensor_id,
							sensor_info['temperature'], '°C',
							{'pressure': sensor_info.get('pressure'), 'battery': sensor_info.get('battery')}
						)

			# Store GPS data
			if data.get('gps'):
				gps_data = data['gps']
				if 'latitude' in gps_data and 'longitude' in gps_data:
					await self.store_sensor_reading(
						'gps_position', 'main',
						gps_data.get('speed', 0.0), 'm/s',
						{
							'latitude': gps_data['latitude'],
							'longitude': gps_data['longitude'],
							'altitude': gps_data.get('altitude'),
							'heading': gps_data.get('heading'),
							'satellites': gps_data.get('satellites')
						}
					)

			# Store environmental data
			if data.get('environmental'):
				env_data = data['environmental']
				for sensor_id, sensor_info in env_data.get('sensors', {}).items():
					if 'temperature' in sensor_info:
						await self.store_sensor_reading(
							'environmental_temperature', sensor_id,
							sensor_info['temperature'], '°C',
							{'humidity': sensor_info.get('humidity')}
						)
					if 'humidity' in sensor_info:
						await self.store_sensor_reading(
							'environmental_humidity', sensor_id,
							sensor_info['humidity'], '%',
							{'temperature': sensor_info.get('temperature')}
						)

			# Store power data
			if data.get('power'):
				power_data = data['power']
				if 'battery' in power_data:
					battery = power_data['battery']
					if 'voltage' in battery:
						await self.store_sensor_reading(
							'battery_voltage', 'main',
							battery['voltage'], 'V',
							{'current': battery.get('current'), 'status': battery.get('status')}
						)
					if 'current' in battery:
						await self.store_sensor_reading(
							'battery_current', 'main',
							battery['current'], 'A',
							{'voltage': battery.get('voltage'), 'status': battery.get('status')}
						)

			# Store engine data
			if data.get('engine'):
				engine_data = data['engine']
				for sensor_id, sensor_info in engine_data.get('sensors', {}).items():
					if 'current_value' in sensor_info:
						await self.store_sensor_reading(
							'engine_sensor', sensor_id,
							sensor_info['current_value'], sensor_info.get('unit', ''),
							{'status': sensor_info.get('status'), 'target_value': sensor_info.get('target_value')}
						)

			# Store inclinometer data
			if data.get('inclinometer'):
				incl_data = data['inclinometer']
				if 'pitch' in incl_data:
					await self.store_sensor_reading(
						'inclinometer_pitch', 'main',
						incl_data['pitch'], '°',
						{'roll': incl_data.get('roll'), 'yaw': incl_data.get('yaw')}
					)
				if 'roll' in incl_data:
					await self.store_sensor_reading(
						'inclinometer_roll', 'main',
						incl_data['roll'], '°',
						{'pitch': incl_data.get('pitch'), 'yaw': incl_data.get('yaw')}
					)

		except Exception as e:
			logger.error(f"Error storing sensor data: {e}")

	async def close(self):
		"""Close database connection"""
		if self.db:
			await self.db.close()
			self.db = None
			logger.info("Database connection closed")
