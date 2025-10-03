#!/usr/bin/env python3
"""
Mock Sensor Hub for Jeep Sensor Hub Research
Simulates all sensors and provides REST API + WebSocket interface for development
"""

import asyncio
import json
import logging
import time
from datetime import datetime, timedelta
from typing import Dict, List, Optional, Any
import yaml
from pathlib import Path

from fastapi import FastAPI, WebSocket, WebSocketDisconnect, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
import uvicorn
from pydantic import BaseModel

from src.database.database import DatabaseManager
from src.sensors.tpms_simulator import TPMSSimulator
from src.sensors.gps_simulator import GPSSimulator
from src.sensors.environmental_simulator import EnvironmentalSimulator
from src.sensors.power_simulator import PowerSimulator
from src.sensors.engine_simulator import EngineSimulator
from src.sensors.inclinometer_simulator import InclinometerSimulator
from src.sensors.cooling_simulator import CoolingSimulator
from src.simulation.event_manager import EventManager

# Configure logging
logging.basicConfig(
	level=logging.INFO,
	format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class MockSensorHub:
	def __init__(self, config_path: str = "config.yaml"):
		self.config = self.load_config(config_path)
		self.db = DatabaseManager(self.config['database']['path'])
		self.websocket_connections: List[WebSocket] = []

		# Initialize sensor simulators
		self.tpms = TPMSSimulator(self.config['tpms'])
		self.gps = GPSSimulator(self.config['gps'])
		self.environmental = EnvironmentalSimulator(self.config['environmental'])
		self.power = PowerSimulator(self.config['power'])
		self.engine = EngineSimulator(self.config['engine'])
		self.inclinometer = InclinometerSimulator(self.config['inclinometer'])
		self.cooling = CoolingSimulator(self.config)

		# Initialize event manager
		try:
			self.event_manager = EventManager(self.config['simulation'])
			logger.info("Event manager initialized successfully")
		except Exception as e:
			logger.error(f"Failed to initialize event manager: {e}")
			import traceback
			logger.error(f"Event manager traceback: {traceback.format_exc()}")
			self.event_manager = None

		# State tracking
		self.engine_state = self.config['general']['engine_state']
		self.power_source = self.config['general']['power_source']
		self.simulation_running = False

	def load_config(self, config_path: str) -> dict:
		"""Load configuration from YAML file"""
		try:
			with open(config_path, 'r') as file:
				return yaml.safe_load(file)
		except FileNotFoundError:
			logger.error(f"Config file {config_path} not found, using defaults")
			return self.get_default_config()

	def get_default_config(self) -> dict:
		"""Return default configuration if config file is missing"""
		return {
			'general': {'engine_state': False, 'power_source': 'house_battery'},
			'tpms': {'enabled': True, 'sensors': []},
			'gps': {'enabled': True, 'simulation_mode': 'static'},
			'environmental': {'enabled': True, 'sensors': {}},
			'power': {'enabled': True},
			'engine': {'enabled': True},
			'inclinometer': {'enabled': True},
			'database': {'path': 'sensor_data.db'},
			'api': {'host': '0.0.0.0', 'port': 8000}
		}

	async def start_simulation(self):
		"""Start the sensor simulation loop"""
		self.simulation_running = True
		logger.info("Starting sensor simulation...")

		while self.simulation_running:
			try:
				# Update all sensors
				await self.update_sensors()

				# Process events
				if self.event_manager:
					await self.event_manager.process_events()

				# Broadcast updates to WebSocket clients
				await self.broadcast_sensor_data()

				# Store data in database
				await self.store_sensor_data()

				# Wait for next update cycle
				await asyncio.sleep(1.0 / self.config['general']['simulation_speed'])

			except Exception as e:
				logger.error(f"Error in simulation loop: {e}")
				import traceback
				logger.error(f"Traceback: {traceback.format_exc()}")
				await asyncio.sleep(1)

	async def update_sensors(self):
		"""Update all sensor values"""
		current_time = datetime.now()

		# Update TPMS sensors
		if self.config['tpms']['enabled']:
			await self.tpms.update(current_time)

		# Update GPS
		if self.config['gps']['enabled']:
			await self.gps.update(current_time)

		# Update environmental sensors
		if self.config['environmental']['enabled']:
			await self.environmental.update(current_time)

		# Update power monitoring
		if self.config['power']['enabled']:
			await self.power.update(current_time, self.engine_state)

		# Update engine sensors
		if self.config['engine']['enabled']:
			await self.engine.update(current_time, self.engine_state)

		# Update inclinometer
		if self.config['inclinometer']['enabled']:
			await self.inclinometer.update(current_time)

		# Update cooling system
		if self.config.get('cooling', {}).get('enabled', False):
			await self.cooling.update(current_time, self.engine_state)

	async def broadcast_sensor_data(self):
		"""Broadcast sensor data to all WebSocket clients"""
		if not self.websocket_connections:
			return

		sensor_data = {
			'timestamp': datetime.now().isoformat(),
			'engine_state': self.engine_state,
			'power_source': self.power_source,
			'tpms': self.tpms.get_data() if self.config['tpms']['enabled'] else None,
			'gps': self.gps.get_data() if self.config['gps']['enabled'] else None,
			'environmental': self.environmental.get_data() if self.config['environmental']['enabled'] else None,
			'power': self.power.get_data() if self.config['power']['enabled'] else None,
			'engine': self.engine.get_data() if self.config['engine']['enabled'] else None,
			'inclinometer': self.inclinometer.get_data() if self.config['inclinometer']['enabled'] else None,
			'cooling': self.cooling.get_status() if self.config.get('cooling', {}).get('enabled', False) else None
		}

		message = json.dumps(sensor_data)
		disconnected = []

		for connection in self.websocket_connections:
			try:
				await connection.send_text(message)
			except WebSocketDisconnect:
				disconnected.append(connection)
			except Exception as e:
				logger.error(f"Error sending to WebSocket: {e}")
				disconnected.append(connection)

		# Remove disconnected clients
		for connection in disconnected:
			self.websocket_connections.remove(connection)

	async def store_sensor_data(self):
		"""Store current sensor data in database"""
		try:
			data = {
				'timestamp': datetime.now(),
				'tpms': self.tpms.get_data() if self.config['tpms']['enabled'] else None,
				'gps': self.gps.get_data() if self.config['gps']['enabled'] else None,
				'environmental': self.environmental.get_data() if self.config['environmental']['enabled'] else None,
				'power': self.power.get_data() if self.config['power']['enabled'] else None,
				'engine': self.engine.get_data() if self.config['engine']['enabled'] else None,
				'inclinometer': self.inclinometer.get_data() if self.config['inclinometer']['enabled'] else None,
				'cooling': self.cooling.get_status() if self.config.get('cooling', {}).get('enabled', False) else None
			}

			# Store in database
			await self.db.store_sensor_data(data)

		except Exception as e:
			logger.error(f"Error storing sensor data: {e}")

	async def stop_simulation(self):
		"""Stop the sensor simulation loop"""
		self.simulation_running = False
		logger.info("Sensor simulation stopped")

	async def get_system_status(self) -> dict:
		"""Get overall system status"""
		return {
			'timestamp': datetime.now().isoformat(),
			'simulation_running': self.simulation_running,
			'engine_state': self.engine_state,
			'power_source': self.power_source,
			'websocket_clients': len(self.websocket_connections),
			'sensors': {
				'tpms': self.config['tpms']['enabled'],
				'gps': self.config['gps']['enabled'],
				'environmental': self.config['environmental']['enabled'],
				'power': self.config['power']['enabled'],
				'engine': self.config['engine']['enabled'],
				'inclinometer': self.config['inclinometer']['enabled'],
				'cooling': self.config.get('cooling', {}).get('enabled', False)
			}
		}

	async def set_engine_state(self, state: bool):
		"""Set engine state (on/off)"""
		self.engine_state = state
		logger.info(f"Engine state changed to: {'ON' if state else 'OFF'}")

	async def set_power_source(self, source: str):
		"""Set power source (engine_battery/house_battery)"""
		if source in ['engine_battery', 'house_battery']:
			self.power_source = source
			logger.info(f"Power source changed to: {source}")
		else:
			raise ValueError("Invalid power source. Must be 'engine_battery' or 'house_battery'")

	async def add_websocket_connection(self, websocket: WebSocket):
		"""Add a new WebSocket connection"""
		self.websocket_connections.append(websocket)
		logger.info(f"WebSocket client connected. Total clients: {len(self.websocket_connections)}")

	async def remove_websocket_connection(self, websocket: WebSocket):
		"""Remove a WebSocket connection"""
		if websocket in self.websocket_connections:
			self.websocket_connections.remove(websocket)
			logger.info(f"WebSocket client disconnected. Total clients: {len(self.websocket_connections)}")

# FastAPI app instance
app = FastAPI(
	title="Mock Sensor Hub API",
	description="Simulated sensor data for Jeep Sensor Hub development",
	version="1.0.0"
)

# Add CORS middleware
app.add_middleware(
	CORSMiddleware,
	allow_origins=["*"],
	allow_credentials=True,
	allow_methods=["*"],
	allow_headers=["*"],
)

# Global sensor hub instance
sensor_hub: Optional[MockSensorHub] = None

@app.on_event("startup")
async def startup_event():
	"""Initialize the sensor hub on startup"""
	global sensor_hub
	sensor_hub = MockSensorHub()
	logger.info("Mock Sensor Hub initialized")

@app.on_event("shutdown")
async def shutdown_event():
	"""Cleanup on shutdown"""
	global sensor_hub
	if sensor_hub:
		await sensor_hub.stop_simulation()
		logger.info("Mock Sensor Hub shutdown complete")

# API Routes
@app.get("/")
async def root():
	"""Root endpoint"""
	return {"message": "Mock Sensor Hub API", "version": "1.0.0"}

@app.get("/status")
async def get_status():
	"""Get system status"""
	if not sensor_hub:
		raise HTTPException(status_code=503, detail="Sensor hub not initialized")
	return await sensor_hub.get_system_status()

@app.get("/sensors/tpms")
async def get_tpms_data():
	"""Get TPMS sensor data"""
	if not sensor_hub:
		raise HTTPException(status_code=503, detail="Sensor hub not initialized")
	return sensor_hub.tpms.get_data()

@app.get("/sensors/gps")
async def get_gps_data():
	"""Get GPS data"""
	if not sensor_hub:
		raise HTTPException(status_code=503, detail="Sensor hub not initialized")
	return sensor_hub.gps.get_data()

@app.get("/sensors/environmental")
async def get_environmental_data():
	"""Get environmental sensor data"""
	if not sensor_hub:
		raise HTTPException(status_code=503, detail="Sensor hub not initialized")
	return sensor_hub.environmental.get_data()

@app.get("/sensors/power")
async def get_power_data():
	"""Get power monitoring data"""
	if not sensor_hub:
		raise HTTPException(status_code=503, detail="Sensor hub not initialized")
	return sensor_hub.power.get_data()

@app.get("/sensors/engine")
async def get_engine_data():
	"""Get engine sensor data"""
	if not sensor_hub:
		raise HTTPException(status_code=503, detail="Sensor hub not initialized")
	return sensor_hub.engine.get_data()

@app.get("/sensors/inclinometer")
async def get_inclinometer_data():
	"""Get inclinometer data"""
	if not sensor_hub:
		raise HTTPException(status_code=503, detail="Sensor hub not initialized")
	return sensor_hub.inclinometer.get_data()

@app.get("/sensors/cooling")
async def get_cooling_data():
	"""Get cooling system data"""
	if not sensor_hub:
		raise HTTPException(status_code=503, detail="Sensor hub not initialized")
	return sensor_hub.cooling.get_status()

@app.post("/control/engine")
async def set_engine_state(engine_state: dict):
	"""Set engine state"""
	if not sensor_hub:
		raise HTTPException(status_code=503, detail="Sensor hub not initialized")

	state = engine_state.get('state', False)
	await sensor_hub.set_engine_state(state)
	return {"message": f"Engine state set to: {'ON' if state else 'OFF'}"}

@app.post("/control/power")
async def set_power_source(power_source: dict):
	"""Set power source"""
	if not sensor_hub:
		raise HTTPException(status_code=503, detail="Sensor hub not initialized")

	source = power_source.get('source', 'house_battery')
	await sensor_hub.set_power_source(source)
	return {"message": f"Power source set to: {source}"}

@app.post("/simulation/start")
async def start_simulation():
	"""Start sensor simulation"""
	if not sensor_hub:
		raise HTTPException(status_code=503, detail="Sensor hub not initialized")

	asyncio.create_task(sensor_hub.start_simulation())
	return {"message": "Sensor simulation started"}

@app.post("/simulation/stop")
async def stop_simulation():
	"""Stop sensor simulation"""
	if not sensor_hub:
		raise HTTPException(status_code=503, detail="Sensor hub not initialized")

	await sensor_hub.stop_simulation()
	return {"message": "Sensor simulation stopped"}

# WebSocket endpoint
@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
	"""WebSocket endpoint for real-time sensor data"""
	await websocket.accept()

	if sensor_hub:
		await sensor_hub.add_websocket_connection(websocket)

		try:
			while True:
				# Keep connection alive
				await websocket.receive_text()
		except WebSocketDisconnect:
			pass
		finally:
			await sensor_hub.remove_websocket_connection(websocket)

if __name__ == "__main__":
	import uvicorn
	uvicorn.run(app, host="0.0.0.0", port=8000)
