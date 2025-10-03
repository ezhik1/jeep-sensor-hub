#!/usr/bin/env python3
"""
Example Integration: Environmental Manager with Sensor Hub
Shows how to integrate the environmental sensor monitoring into the main application
"""

import asyncio
import yaml
import logging
from pathlib import Path
from src.sensors.environmental_manager_dev import EnvironmentalManagerDev

# Configure logging
logging.basicConfig(
	level=logging.INFO,
	format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)

logger = logging.getLogger(__name__)

class SensorHubExample:
	"""Example sensor hub with environmental monitoring"""

	def __init__(self, config_path: str = "config.yaml"):
		"""Initialize the sensor hub example"""
		self.config_path = config_path
		self.config = self._load_config()
		self.environmental_manager = None
		self.running = False

		logger.info("Sensor Hub Example initialized")

	def _load_config(self) -> dict:
		"""Load configuration from YAML file"""
		try:
			with open(self.config_path, 'r') as f:
				config = yaml.safe_load(f)
			logger.info(f"Configuration loaded from {self.config_path}")
			return config
		except Exception as e:
			logger.error(f"Error loading configuration: {e}")
			raise

	async def start(self):
		"""Start the sensor hub"""
		try:
			logger.info("Starting Sensor Hub...")

			# Initialize environmental manager
			if self.config.get('environmental', {}).get('enabled', False):
				logger.info("Initializing environmental monitoring...")
				self.environmental_manager = EnvironmentalManagerDev(
					self.config['environmental']
				)

				# Start environmental monitoring
				await self.environmental_manager.start_monitoring()
				logger.info("Environmental monitoring started")
			else:
				logger.warning("Environmental monitoring is disabled in config")

			self.running = True
			logger.info("Sensor Hub started successfully")

		except Exception as e:
			logger.error(f"Error starting Sensor Hub: {e}")
			raise

	async def stop(self):
		"""Stop the sensor hub"""
		try:
			logger.info("Stopping Sensor Hub...")

			# Stop environmental monitoring
			if self.environmental_manager:
				await self.environmental_manager.stop_monitoring()
				self.environmental_manager.cleanup()
				logger.info("Environmental monitoring stopped")

			self.running = False
			logger.info("Sensor Hub stopped")

		except Exception as e:
			logger.error(f"Error stopping Sensor Hub: {e}")

	async def get_system_status(self) -> dict:
		"""Get overall system status"""
		try:
			status = {
				'running': self.running,
				'timestamp': asyncio.get_event_loop().time(),
				'environmental': {}
			}

			# Get environmental status
			if self.environmental_manager:
				status['environmental'] = {
					'enabled': True,
					'sensor_status': self.environmental_manager.get_sensor_status(),
					'statistics': self.environmental_manager.get_statistics(),
					'current_readings': self.environmental_manager.get_current_readings(),
					'active_alerts': self.environmental_manager.get_active_alerts()
				}
			else:
				status['environmental'] = {
					'enabled': False,
					'message': 'Environmental monitoring not initialized'
				}

			return status

		except Exception as e:
			logger.error(f"Error getting system status: {e}")
			return {
				'running': self.running,
				'error': str(e),
				'timestamp': asyncio.get_event_loop().time()
			}

	async def run_monitoring_loop(self, duration: int = 60):
		"""Run the monitoring loop for a specified duration"""
		try:
			logger.info(f"Starting monitoring loop for {duration} seconds...")
			start_time = asyncio.get_event_loop().time()

			while asyncio.get_event_loop().time() - start_time < duration:
				if not self.running:
					break

				# Get current status
				status = await self.get_system_status()

				# Log environmental readings
				if status['environmental'].get('enabled', False):
					readings = status['environmental'].get('current_readings', {})
					if readings:
						for location, reading in readings.items():
							if reading:
								logger.info(f"{location.upper()}: {reading.temperature:.1f}°C, {reading.humidity:.1f}%")
							else:
								logger.warning(f"{location.upper()}: No data")

					# Check for alerts
					alerts = status['environmental'].get('active_alerts', [])
					if alerts:
						for alert in alerts:
							logger.warning(f"Alert: {alert['type']} - {alert['message']}")

				# Wait before next check
				await asyncio.sleep(5)

			logger.info("Monitoring loop completed")

		except Exception as e:
			logger.error(f"Error in monitoring loop: {e}")

async def main():
	"""Main example function"""
	try:
		# Create sensor hub instance
		sensor_hub = SensorHubExample()

		# Start the sensor hub
		await sensor_hub.start()

		# Run monitoring for 30 seconds
		await sensor_hub.run_monitoring_loop(30)

		# Stop the sensor hub
		await sensor_hub.stop()

		logger.info("Example completed successfully")

	except Exception as e:
		logger.error(f"Example failed: {e}")
		import traceback
		traceback.print_exc()

if __name__ == "__main__":
	asyncio.run(main())
