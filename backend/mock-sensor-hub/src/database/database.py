#!/usr/bin/env python3
"""
Database management for Mock Sensor Hub
Handles SQLite database operations for sensor data, routes, and events
"""

import asyncio
import aiosqlite
import logging
from datetime import datetime
from typing import Dict, List, Optional, Any
from pathlib import Path
import json

logger = logging.getLogger(__name__)

class DatabaseManager:
	def __init__(self, db_path: str):
		self.db_path = db_path
		self.initialized = False

	async def initialize(self):
		"""Initialize database and create tables"""
		try:
			async with aiosqlite.connect(self.db_path) as db:
				await self._create_tables(db)
			self.initialized = True
			logger.info(f"Database initialized: {self.db_path}")
		except Exception as e:
			logger.error(f"Failed to initialize database: {e}")
			raise

	async def _create_tables(self, db):
		"""Create database tables if they don't exist"""
		# Sensor readings table
		await db.execute("""
		CREATE TABLE IF NOT EXISTS sensor_readings (
		id INTEGER PRIMARY KEY AUTOINCREMENT,
		timestamp DATETIME NOT NULL,
		sensor_type TEXT NOT NULL,
		sensor_id TEXT,
		value REAL,
		unit TEXT,
		metadata TEXT
		)
		""")

		# Routes table
		await db.execute("""
		CREATE TABLE IF NOT EXISTS routes (
		id INTEGER PRIMARY KEY AUTOINCREMENT,
		start_time DATETIME NOT NULL,
		end_time DATETIME,
		distance REAL,
		max_speed REAL,
		avg_speed REAL,
		engine_cycles INTEGER DEFAULT 0,
		metadata TEXT
		)
		""")

		# Route points table
		await db.execute("""
		CREATE TABLE IF NOT EXISTS route_points (
		id INTEGER PRIMARY KEY AUTOINCREMENT,
		route_id INTEGER,
		timestamp DATETIME NOT NULL,
		latitude REAL,
		longitude REAL,
		altitude REAL,
		speed REAL,
		heading REAL,
		FOREIGN KEY (route_id) REFERENCES routes (id)
		)
		""")

		# Engine events table
		await db.execute("""
		CREATE TABLE IF NOT EXISTS engine_events (
		id INTEGER PRIMARY KEY AUTOINCREMENT,
		timestamp DATETIME NOT NULL,
		event_type TEXT NOT NULL,
		metadata TEXT
		)
		""")

		# System stats table
		await db.execute("""
		CREATE TABLE IF NOT EXISTS system_stats (
		id INTEGER PRIMARY KEY AUTOINCREMENT,
		timestamp DATETIME NOT NULL,
		uptime_seconds INTEGER,
		engine_cycles INTEGER,
		total_distance REAL,
		avg_fuel_efficiency REAL,
		metadata TEXT
		)
		""")

		await db.commit()

	async def store_sensor_reading(self, sensor_type: str, sensor_id: str,
			value: float, unit: str, metadata: Dict = None):
		"""Store a single sensor reading"""
		if not self.initialized:
			await self.initialize()

		try:
			async with aiosqlite.connect(self.db_path) as db:
				await db.execute("""
				INSERT INTO sensor_readings (timestamp, sensor_type, sensor_id, value, unit, metadata)
				VALUES (?, ?, ?, ?, ?, ?)
				""", (
					datetime.now().isoformat(),
					sensor_type,
					sensor_id,
					value,
					unit,
					json.dumps(metadata) if metadata else None
				))
				await db.commit()
		except Exception as e:
			logger.error(f"Failed to store sensor reading: {e}")

	async def store_route_point(self, route_id: int, latitude: float, longitude: float,
			altitude: float, speed: float, heading: float):
		"""Store a GPS route point"""
		if not self.initialized:
			await self.initialize()

		try:
			async with aiosqlite.connect(self.db_path) as db:
				await db.execute("""
				INSERT INTO route_points (route_id, timestamp, latitude, longitude, altitude, speed, heading)
				VALUES (?, ?, ?, ?, ?, ?, ?)
				""", (
					route_id,
					datetime.now().isoformat(),
					latitude,
					longitude,
					altitude,
					speed,
					heading
				))
				await db.commit()
		except Exception as e:
			logger.error(f"Failed to store route point: {e}")

	async def create_route(self, start_time: datetime) -> int:
		"""Create a new route and return its ID"""
		if not self.initialized:
			await self.initialize()

		try:
			async with aiosqlite.connect(self.db_path) as db:
				cursor = await db.execute("""
				INSERT INTO routes (start_time, metadata)
				VALUES (?, ?)
				""", (
					start_time.isoformat(),
					json.dumps({"created_by": "mock_sensor_hub"})
				))
				await db.commit()
				return cursor.lastrowid
		except Exception as e:
			logger.error(f"Failed to create route: {e}")
			return None

	async def update_route(self, route_id: int, end_time: datetime = None,
		distance: float = None, max_speed: float = None,
			avg_speed: float = None, engine_cycles: int = None):
		"""Update route information"""
		if not self.initialized:
			await self.initialize()

		try:
			async with aiosqlite.connect(self.db_path) as db:
				update_fields = []
				params = []

				if end_time:
					update_fields.append("end_time = ?")
					params.append(end_time.isoformat())
				if distance is not None:
					update_fields.append("distance = ?")
					params.append(distance)
				if max_speed is not None:
					update_fields.append("max_speed = ?")
					params.append(max_speed)
				if avg_speed is not None:
					update_fields.append("avg_speed = ?")
					params.append(avg_speed)
				if engine_cycles is not None:
					update_fields.append("engine_cycles = ?")
					params.append(engine_cycles)

				if update_fields:
					params.append(route_id)
					await db.execute(f"""
					UPDATE routes SET {', '.join(update_fields)}
					WHERE id = ?
					""", params)
					await db.commit()
		except Exception as e:
			logger.error(f"Failed to update route: {e}")

	async def store_engine_event(self, event_type: str, metadata: Dict = None):
		"""Store an engine event"""
		if not self.initialized:
			await self.initialize()

		try:
			async with aiosqlite.connect(self.db_path) as db:
				await db.execute("""
				INSERT INTO engine_events (timestamp, event_type, metadata)
				VALUES (?, ?, ?)
				""", (
					datetime.now().isoformat(),
					event_type,
					json.dumps(metadata) if metadata else None
				))
				await db.commit()
		except Exception as e:
			logger.error(f"Failed to store engine event: {e}")

	async def get_sensor_history(self, sensor_type: str, sensor_id: str = None,
		start_time: datetime = None, end_time: datetime = None,
			limit: int = 1000) -> List[Dict]:
		"""Get sensor reading history"""
		if not self.initialized:
			await self.initialize()

		try:
			async with aiosqlite.connect(self.db_path) as db:
				query = """
				SELECT timestamp, sensor_type, sensor_id, value, unit, metadata
				FROM sensor_readings
				WHERE sensor_type = ?
				"""
				params = [sensor_type]

				if sensor_id:
					query += " AND sensor_id = ?"
					params.append(sensor_id)
				if start_time:
					query += " AND timestamp >= ?"
					params.append(start_time.isoformat())
				if end_time:
					query += " AND timestamp <= ?"
					params.append(end_time.isoformat())

				query += " ORDER BY timestamp DESC LIMIT ?"
				params.append(limit)

				async with db.execute(query, params) as cursor:
					rows = await cursor.fetchall()
					return [
						{
							'timestamp': row[0],
							'sensor_type': row[1],
							'sensor_id': row[2],
							'value': row[3],
							'unit': row[4],
							'metadata': json.loads(row[5]) if row[5] else None
						}
						for row in rows
					]
		except Exception as e:
			logger.error(f"Failed to get sensor history: {e}")
			return []

	async def get_routes(self, start_time: datetime = None, end_time: datetime = None) -> List[Dict]:
		"""Get route history"""
		if not self.initialized:
			await self.initialize()

		try:
			async with aiosqlite.connect(self.db_path) as db:
				query = "SELECT * FROM routes WHERE 1=1"
				params = []

				if start_time:
					query += " AND start_time >= ?"
					params.append(start_time.isoformat())
				if end_time:
					query += " AND end_time <= ?"
					params.append(end_time.isoformat())

				query += " ORDER BY start_time DESC"

				async with db.execute(query, params) as cursor:
					rows = await cursor.fetchall()
					columns = [description[0] for description in cursor.description]
					return [dict(zip(columns, row)) for row in rows]
		except Exception as e:
			logger.error(f"Failed to get routes: {e}")
			return []

	async def export_data(self, format: str = 'csv', start_time: datetime = None,
			end_time: datetime = None) -> str:
		"""Export sensor data in specified format"""
		if not self.initialized:
			await self.initialize()

		try:
			if format.lower() == 'csv':
				return await self._export_csv(start_time, end_time)
			elif format.lower() == 'json':
				return await self._export_json(start_time, end_time)
			else:
				raise ValueError(f"Unsupported export format: {format}")
		except Exception as e:
			logger.error(f"Failed to export data: {e}")
			return ""

	async def _export_csv(self, start_time: datetime, end_time: datetime) -> str:
		"""Export data as CSV"""
		try:
			data = await self.get_sensor_history('all', 24)  # Get last 24 hours
			if not data:
				return "timestamp,sensor_type,sensor_id,value,unit,metadata\n"

			csv_content = "timestamp,sensor_type,sensor_id,value,unit,metadata\n"
			for row in data:
				csv_content += f"{row['timestamp']},{row['sensor_type']},{row['sensor_id']},{row['value']},{row['unit']},{row['metadata']}\n"
			return csv_content
		except Exception as e:
			logger.error(f"Failed to export CSV: {e}")
			return ""

	async def _export_json(self, start_time: datetime, end_time: datetime) -> str:
		"""Export data as JSON"""
		try:
			data = await self.get_sensor_history('all', 24)  # Get last 24 hours
			import json
			return json.dumps(data, default=str)
		except Exception as e:
			logger.error(f"Failed to export JSON: {e}")
			return "[]"

	async def clear_data(self, before_date: datetime = None):
		"""Clear old data from database"""
		if not self.initialized:
			await self.initialize()

		try:
			async with aiosqlite.connect(self.db_path) as db:
				if before_date:
					await db.execute("DELETE FROM sensor_readings WHERE timestamp < ?",
					(before_date.isoformat(),))
					await db.execute("DELETE FROM route_points WHERE timestamp < ?",
					(before_date.isoformat(),))
					await db.execute("DELETE FROM engine_events WHERE timestamp < ?",
					(before_date.isoformat(),))
				else:
					await db.execute("DELETE FROM sensor_readings")
					await db.execute("DELETE FROM route_points")
					await db.execute("DELETE FROM engine_events")
					await db.execute("DELETE FROM routes")
					await db.execute("DELETE FROM system_stats")

				await db.commit()
				logger.info("Database cleared successfully")
		except Exception as e:
			logger.error(f"Failed to clear database: {e}")

	async def store_sensor_data(self, data: Dict):
		"""Store sensor data in database"""
		if not self.initialized:
			await self.initialize()

		try:
			async with aiosqlite.connect(self.db_path) as db:
				timestamp = data['timestamp']

				# Store TPMS data
				if data.get('tpms'):
					for sensor in data['tpms']:
						await db.execute("""
						INSERT INTO sensor_readings (timestamp, sensor_type, sensor_id, value, unit, metadata)
						VALUES (?, ?, ?, ?, ?, ?)
						""", (
							timestamp.isoformat(),
							'tpms',
							sensor['sensor_id'],
							sensor['pressure'],
							'PSI',
							json.dumps(sensor)
						))

				# Store GPS data
				if data.get('gps'):
					gps = data['gps']
					await db.execute("""
					INSERT INTO sensor_readings (timestamp, sensor_type, sensor_id, value, unit, metadata)
					VALUES (?, ?, ?, ?, ?, ?)
					""", (
						timestamp.isoformat(),
						'gps',
						'gps_main',
						gps.get('speed', 0),
						'm/s',
						json.dumps(gps)
					))

				# Store environmental data
				if data.get('environmental'):
					for sensor in data['environmental']:
						await db.execute("""
						INSERT INTO sensor_readings (timestamp, sensor_type, sensor_id, value, unit, metadata)
						VALUES (?, ?, ?, ?, ?, ?)
						""", (
							timestamp.isoformat(),
							'environmental',
							sensor['sensor_id'],
							sensor['current_value'],
							sensor['unit'],
							json.dumps(sensor)
						))

				# Store power data
				if data.get('power'):
					for system in data['power']:
						await db.execute("""
						INSERT INTO sensor_readings (timestamp, sensor_type, sensor_id, value, unit, metadata)
						VALUES (?, ?, ?, ?, ?, ?)
						""", (
							timestamp.isoformat(),
							'power',
							system['system_id'],
							system['voltage'],
							'V',
							json.dumps(system)
						))

				# Store engine data
				if data.get('engine'):
					for sensor in data['engine']:
						await db.execute("""
						INSERT INTO sensor_readings (timestamp, sensor_type, sensor_id, value, unit, metadata)
						VALUES (?, ?, ?, ?, ?, ?)
						""", (
							timestamp.isoformat(),
							'engine',
							sensor['sensor_id'],
							sensor['current_value'],
							sensor['unit'],
							json.dumps(sensor)
						))

				# Store inclinometer data
				if data.get('inclinometer'):
					incl = data['inclinometer']
					await db.execute("""
					INSERT INTO sensor_readings (timestamp, sensor_type, sensor_id, value, unit, metadata)
					VALUES (?, ?, ?, ?, ?, ?)
					""", (
						timestamp.isoformat(),
						'inclinometer',
						'mpu6050',
						incl.get('pitch', 0),
						'degrees',
						json.dumps(incl)
					))

				await db.commit()
				logger.debug("Sensor data stored successfully")

		except Exception as e:
			logger.error(f"Failed to store sensor data: {e}")

	async def close(self):
		"""Close database connections"""
		# aiosqlite handles connection cleanup automatically
		# This method is here for compatibility with the main application
		logger.info("Database manager closed")
