#!/usr/bin/env python3
"""
Raspberry Pi Zero Sensor Hub for Jeep Sensor Hub Research
Main application for hardware sensor interfacing and data collection
"""

import asyncio
import json
import logging
import signal
import sys
import time
from datetime import datetime, timedelta
from typing import Dict, List, Optional, Any
import yaml
from pathlib import Path

import RPi.GPIO as GPIO
from fastapi import FastAPI, WebSocket, WebSocketDisconnect, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
import uvicorn

from database.database import DatabaseManager
from sensors.tpms_manager import TPMSManager
from sensors.gps_manager import GPSManager
from sensors.temp_humidity import TempHumidityManager
from sensors.power_monitor import PowerMonitor
from sensors.engine_sensors import EngineSensors
from sensors.inclinometer import Inclinometer
from communication.serial_manager import SerialManager
from communication.wifi_manager import WiFiManager
from power_management import PowerManager

# Configure logging
logging.basicConfig(
	level=logging.INFO,
	format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
	handlers=[
		logging.FileHandler('/var/log/jeep-sensor-hub.log'),
		logging.StreamHandler(sys.stdout)
	]
)
logger = logging.getLogger(__name__)

class SensorHub:
	def __init__(self, config_path: str = "config.yaml"):
		self.config = self.load_config(config_path)
		self.db = DatabaseManager(self.config['database']['path'])
		self.websocket_connections: List[WebSocket] = []

		# Initialize GPIO
		GPIO.setmode(GPIO.BCM)
		GPIO.setwarnings(False)

		# Initialize power management
		self.power_manager = PowerManager(self.config['power_management'])

		# Initialize sensor managers
		self.tpms = TPMSManager(self.config['tpms']) if self.config['tpms']['enabled'] else None
		self.gps = GPSManager(self.config['gps']) if self.config['gps']['enabled'] else None
		self.environmental = TempHumidityManager(self.config['environmental']) if self.config['environmental']['enabled'] else None
		self.power = PowerMonitor(self.config['power']) if self.config['power']['enabled'] else None
		self.engine = EngineSensors(self.config['engine']) if self.config['engine']['enabled'] else None
		self.inclinometer = Inclinometer(self.config['inclinometer']) if self.config['inclinometer']['enabled'] else None

		# Initialize communication managers
		self.serial_manager = SerialManager(self.config['communication']['serial'])
		self.wifi_manager = WiFiManager(self.config['communication']['wifi'])

		# State tracking
		self.engine_state = False
		self.power_source = self.config['general']['power_source']
		self.sensor_hub_running = False

		# Setup GPIO pins
		self.setup_gpio()

		# Route tracking
		self.current_route = None
		self.route_start_time = None
		self.route_history = []

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
			'database': {'path': '/home/pi/sensor_data.db'},
			'api': {'host': '0.0.0.0', 'port': 8000}
		}

	def setup_gpio(self):
		"""Setup GPIO pins for inputs and outputs"""
		try:
			# Engine signal input
			engine_pin = self.config['general']['engine_signal_pin']
			GPIO.setup(engine_pin, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
			GPIO.add_event_detect(engine_pin, GPIO.BOTH, callback=self.engine_state_changed)

			# Power source switch input
			power_switch_pin = self.config['general']['power_source_switch_pin']
			GPIO.setup(power_switch_pin, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
			GPIO.add_event_detect(power_switch_pin, GPIO.BOTH, callback=self.power_source_changed)

			# System status LED
			led_pin = self.config['general']['system_led_pin']
			GPIO.setup(led_pin, GPIO.OUT)
			GPIO.output(led_pin, GPIO.HIGH)

			# Digital I/O outputs
			for component in self.config['digital_io']['components']:
				pin = component['pin']
				GPIO.setup(pin, GPIO.OUT)
				if component.get('pwm_enabled', False):
					pwm = GPIO.PWM(pin, component['pwm_frequency'])
					pwm.start(0)
					component['pwm'] = pwm
				else:
					GPIO.output(pin, GPIO.LOW)

			logger.info("GPIO setup completed successfully")

		except Exception as e:
			logger.error(f"Error setting up GPIO: {e}")

	def engine_state_changed(self, channel):
		"""Callback for engine state changes"""
		try:
			new_state = GPIO.input(self.config['general']['engine_signal_pin'])
			if new_state != self.engine_state:
				self.engine_state = new_state
				logger.info(f"Engine state changed to: {'ON' if self.engine_state else 'OFF'}")

				if self.engine_state:
					# Engine started - start new route
					self.start_new_route()
				else:
					# Engine stopped - end current route
					self.end_current_route()

				# Update power management
				self.power_manager.engine_state_changed(self.engine_state)

				# Broadcast state change
				asyncio.create_task(self.broadcast_engine_state())

		except Exception as e:
			logger.error(f"Error handling engine state change: {e}")

	def power_source_changed(self, channel):
		"""Callback for power source changes"""
		try:
			new_source = GPIO.input(self.config['general']['power_source_switch_pin'])
			if new_source:
				self.power_source = "starter_battery"
			else:
				self.power_source = "house_battery"

			logger.info(f"Power source changed to: {self.power_source}")

			# Update power management
			self.power_manager.power_source_changed(self.power_source)

		except Exception as e:
			logger.error(f"Error handling power source change: {e}")

	def start_new_route(self):
		"""Start tracking a new route when engine starts"""
		if self.gps and self.gps.has_fix():
			self.current_route = {
				'start_time': datetime.now(),
				'start_location': self.gps.get_current_location(),
				'waypoints': [],
				'total_distance': 0.0,
				'max_speed': 0.0,
				'avg_speed': 0.0
			}
			self.route_start_time = datetime.now()
			logger.info("New route started")

	def end_current_route(self):
		"""End current route tracking when engine stops"""
		if self.current_route and self.gps:
			self.current_route['end_time'] = datetime.now()
			self.current_route['end_location'] = self.gps.get_current_location()
			self.current_route['duration'] = (self.current_route['end_time'] - self.current_route['start_time']).total_seconds()

			# Calculate route statistics
			if self.current_route['waypoints']:
				speeds = [wp['speed'] for wp in self.current_route['waypoints'] if wp['speed'] > 0]
				if speeds:
					self.current_route['max_speed'] = max(speeds)
					self.current_route['avg_speed'] = sum(speeds) / len(speeds)

			# Store route in database
			asyncio.create_task(self.store_route_data())

			# Add to history
			self.route_history.append(self.current_route)
			if len(self.route_history) > 100:  # Keep last 100 routes
				self.route_history.pop(0)

			logger.info(f"Route ended. Duration: {self.current_route['duration']:.1f}s, Distance: {self.current_route['total_distance']:.2f}km")
			self.current_route = None
			self.route_start_time = None

	async def store_route_data(self):
		"""Store route data in database"""
		try:
			if self.current_route:
				await self.db.store_route_data(self.current_route)
		except Exception as e:
			logger.error(f"Error storing route data: {e}")

	async def start_sensor_hub(self):
		"""Start the sensor hub main loop"""
		self.sensor_hub_running = True
		logger.info("Starting sensor hub...")

		# Initialize database
		await self.db.initialize()

		# Start communication managers
		await self.serial_manager.start()
		if self.config['communication']['wifi']['enabled']:
			await self.wifi_manager.start()

		# Start sensor monitoring
		asyncio.create_task(self.monitor_sensors())

		# Start route updates
		if self.gps:
			asyncio.create_task(self.update_route_tracking())

		logger.info("Sensor hub started successfully")

		while self.sensor_hub_running:
			try:
				await asyncio.sleep(1)
			except KeyboardInterrupt:
				break
			except Exception as e:
				logger.error(f"Error in sensor hub loop: {e}")
				await asyncio.sleep(5)

	async def monitor_sensors(self):
		"""Monitor all sensors and update data"""
		while self.sensor_hub_running:
			try:
				current_time = datetime.now()

				# Update TPMS sensors
				if self.tpms:
					await self.tpms.update(current_time)

				# Update GPS
				if self.gps:
					await self.gps.update(current_time)

				# Update environmental sensors
				if self.environmental:
					await self.environmental.update(current_time)

				# Update power monitoring
				if self.power:
					await self.power.update(current_time, self.engine_state)

				# Update engine sensors
				if self.engine:
					await self.engine.update(current_time, self.engine_state)

				# Update inclinometer
				if self.inclinometer:
					await self.inclinometer.update(current_time)

				# Store data in database
				await self.store_sensor_data()

				# Broadcast updates to WebSocket clients
				await self.broadcast_sensor_data()

				# Wait for next update cycle
				await asyncio.sleep(1)

			except Exception as e:
				logger.error(f"Error monitoring sensors: {e}")
				await asyncio.sleep(5)

	async def update_route_tracking(self):
		"""Update route tracking with GPS data"""
		while self.sensor_hub_running and self.gps:
			try:
				if self.current_route and self.gps.has_fix():
					location = self.gps.get_current_location()
					speed = self.gps.get_current_speed()
					altitude = self.gps.get_current_altitude()

					waypoint = {
						'timestamp': datetime.now(),
						'latitude': location['latitude'],
						'longitude': location['longitude'],
						'altitude': altitude,
						'speed': speed
					}

					# Calculate distance from previous waypoint
					if self.current_route['waypoints']:
						prev_wp = self.current_route['waypoints'][-1]
						distance = self.gps.calculate_distance(
							prev_wp['latitude'], prev_wp['longitude'],
							location['latitude'], location['longitude']
						)
						self.current_route['total_distance'] += distance

					self.current_route['waypoints'].append(waypoint)

					# Limit waypoints to prevent memory issues
					if len(self.current_route['waypoints']) > 1000:
						self.current_route['waypoints'] = self.current_route['waypoints'][-500:]

					await asyncio.sleep(5)  # Update route every 5 seconds

			except Exception as e:
				logger.error(f"Error updating route tracking: {e}")
				await asyncio.sleep(10)

	async def broadcast_sensor_data(self):
		"""Broadcast sensor data to all WebSocket clients"""
		if not self.websocket_connections:
			return

		sensor_data = {
			'timestamp': datetime.now().isoformat(),
			'engine_state': self.engine_state,
			'power_source': self.power_source,
			'tpms': self.tpms.get_data() if self.tpms else None,
			'gps': self.gps.get_data() if self.gps else None,
			'environmental': self.environmental.get_data() if self.environmental else None,
			'power': self.power.get_data() if self.power else None,
			'engine': self.engine.get_data() if self.engine else None,
			'inclinometer': self.inclinometer.get_data() if self.inclinometer else None,
			'current_route': self.current_route
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

	async def broadcast_engine_state(self):
		"""Broadcast engine state change to WebSocket clients"""
		if not self.websocket_connections:
			return

		state_data = {
			'timestamp': datetime.now().isoformat(),
			'type': 'engine_state_change',
			'engine_state': self.engine_state
		}

		message = json.dumps(state_data)
		for connection in self.websocket_connections:
			try:
				await connection.send_text(message)
			except Exception as e:
				logger.error(f"Error broadcasting engine state: {e}")

	async def store_sensor_data(self):
		"""Store current sensor data in database"""
		try:
			data = {
				'timestamp': datetime.now(),
				'tpms': self.tpms.get_data() if self.tpms else None,
				'gps': self.gps.get_data() if self.gps else None,
				'environmental': self.environmental.get_data() if self.environmental else None,
				'power': self.power.get_data() if self.power else None,
				'engine': self.engine.get_data() if self.engine else None,
				'inclinometer': self.inclinometer.get_data() if self.inclinometer else None
			}

			await self.db.store_sensor_data(data)

		except Exception as e:
			logger.error(f"Error storing sensor data: {e}")

	async def connect_websocket(self, websocket: WebSocket):
		"""Handle new WebSocket connection"""
		await websocket.accept()
		self.websocket_connections.append(websocket)
		logger.info(f"WebSocket connected. Total connections: {len(self.websocket_connections)}")

	def disconnect_websocket(self, websocket: WebSocket):
		"""Handle WebSocket disconnection"""
		if websocket in self.websocket_connections:
			self.websocket_connections.remove(websocket)
			logger.info(f"WebSocket disconnected. Total connections: {len(self.websocket_connections)}")

	async def stop_sensor_hub(self):
		"""Stop the sensor hub"""
		self.sensor_hub_running = False
		logger.info("Stopping sensor hub...")

		# Stop communication managers
		await self.serial_manager.stop()
		if self.config['communication']['wifi']['enabled']:
			await self.wifi_manager.stop()

		# Close database
		await self.db.close()

		# Cleanup GPIO
		GPIO.cleanup()

		logger.info("Sensor hub stopped")

# Global sensor hub instance
sensor_hub: Optional[SensorHub] = None

# Create FastAPI app
app = FastAPI(
	title="Jeep Sensor Hub - Raspberry Pi Zero",
	description="Hardware sensor hub for vehicle monitoring",
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

@app.on_event("startup")
async def startup_event():
	"""Initialize the sensor hub on startup"""
	global sensor_hub
	sensor_hub = SensorHub()

	# Start sensor hub in background
	asyncio.create_task(sensor_hub.start_sensor_hub())
	logger.info("Sensor Hub started successfully")

@app.on_event("shutdown")
async def shutdown_event():
	"""Cleanup on shutdown"""
	global sensor_hub
	if sensor_hub:
		await sensor_hub.stop_sensor_hub()
	logger.info("Sensor Hub shutdown complete")

# WebSocket endpoint for real-time sensor data
@app.websocket("/ws/sensors")
async def websocket_endpoint(websocket: WebSocket):
	"""WebSocket endpoint for real-time sensor data"""
	if not sensor_hub:
		await websocket.close(code=1000, reason="Service not ready")
		return

	await sensor_hub.connect_websocket(websocket)
	try:
		while True:
			# Keep connection alive
			await websocket.receive_text()
	except WebSocketDisconnect:
		sensor_hub.disconnect_websocket(websocket)

# REST API endpoints
@app.get("/")
async def root():
	"""Root endpoint with service information"""
	return {
		"service": "Jeep Sensor Hub - Raspberry Pi Zero",
		"version": "1.0.0",
		"status": "running",
		"timestamp": datetime.now().isoformat()
	}

@app.get("/api/sensors/current")
async def get_current_sensors():
	"""Get current sensor readings"""
	if not sensor_hub:
		raise HTTPException(status_code=503, detail="Service not ready")

	return {
		'timestamp': datetime.now().isoformat(),
		'engine_state': sensor_hub.engine_state,
		'power_source': sensor_hub.power_source,
		'tpms': sensor_hub.tpms.get_data() if sensor_hub.tpms else None,
		'gps': sensor_hub.gps.get_data() if sensor_hub.gps else None,
		'environmental': sensor_hub.environmental.get_data() if sensor_hub.environmental else None,
		'power': sensor_hub.power.get_data() if sensor_hub.power else None,
		'engine': sensor_hub.engine.get_data() if sensor_hub.engine else None,
		'inclinometer': sensor_hub.inclinometer.get_data() if sensor_hub.inclinometer else None,
		'current_route': sensor_hub.current_route
	}

@app.post("/api/engine/toggle")
async def toggle_engine():
	"""Toggle engine state (for testing)"""
	if not sensor_hub:
		raise HTTPException(status_code=503, detail="Service not ready")

	# Simulate engine state change for testing
	sensor_hub.engine_state = not sensor_hub.engine_state
	logger.info(f"Engine state toggled to: {sensor_hub.engine_state}")

	return {"engine_state": sensor_hub.engine_state}

@app.get("/api/routes/history")
async def get_route_history():
	"""Get route history"""
	if not sensor_hub:
		raise HTTPException(status_code=503, detail="Service not ready")

	return {"routes": sensor_hub.route_history}

@app.get("/api/history/{sensor_type}")
async def get_sensor_history(sensor_type: str, hours: int = 24):
	"""Get historical sensor data"""
	if not sensor_hub:
		raise HTTPException(status_code=503, detail="Service not ready")

	try:
		data = await sensor_hub.db.get_sensor_history(sensor_type, hours)
		return {"data": data}
	except Exception as e:
		raise HTTPException(status_code=500, detail=str(e))

@app.post("/api/export/{sensor_type}")
async def export_sensor_data(sensor_type: str, format: str = "json", hours: int = 24):
	"""Export sensor data in specified format"""
	if not sensor_hub:
		raise HTTPException(status_code=503, detail="Service not ready")

	try:
		if format.lower() == "csv":
			data = await sensor_hub.db.export_csv(sensor_type, hours)
			return JSONResponse(content=data, media_type="text/csv")
		else:
			data = await sensor_hub.db.export_json(sensor_type, hours)
			return {"data": data}
	except Exception as e:
		raise HTTPException(status_code=500, detail=str(e))

# Signal handlers for graceful shutdown
def signal_handler(signum, frame):
	"""Handle shutdown signals"""
	logger.info(f"Received signal {signum}, shutting down...")
	if sensor_hub:
		asyncio.create_task(sensor_hub.stop_sensor_hub())
	sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)

if __name__ == "__main__":
	uvicorn.run(
		"main:app",
		host="0.0.0.0",
		port=8000,
		log_level="info"
	)
