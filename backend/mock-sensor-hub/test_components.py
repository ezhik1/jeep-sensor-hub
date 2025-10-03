#!/usr/bin/env python3
"""
Test script to verify all components can be imported and initialized
"""

import asyncio
import yaml
import logging

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

async def test_components():
	"""Test all component imports and initialization"""
	try:
		logger.info("Testing component imports...")

		# Test imports
		from src.database.database import DatabaseManager
		from src.sensors.tpms_simulator import TPMSSimulator
		from src.sensors.gps_simulator import GPSSimulator
		from src.sensors.environmental_simulator import EnvironmentalSimulator
		from src.sensors.power_simulator import PowerSimulator
		from src.sensors.engine_simulator import EngineSimulator
		from src.sensors.inclinometer_simulator import InclinometerSimulator
		from src.simulation.event_manager import EventManager

		logger.info("✅ All imports successful")

		# Load config
		logger.info("Loading configuration...")
		with open('config.yaml', 'r') as f:
			config = yaml.safe_load(f)
		logger.info("✅ Configuration loaded")

		# Test component initialization
		logger.info("Testing component initialization...")

		db = DatabaseManager(config['database']['path'])
		logger.info("✅ DatabaseManager created")

		tpms = TPMSSimulator(config['tpms'])
		logger.info("✅ TPMSSimulator created")

		gps = GPSSimulator(config['gps'])
		logger.info("✅ GPSSimulator created")

		environmental = EnvironmentalSimulator(config['environmental'])
		logger.info("✅ EnvironmentalSimulator created")

		power = PowerSimulator(config['power'])
		logger.info("✅ PowerSimulator created")

		engine = EngineSimulator(config['engine'])
		logger.info("✅ EngineSimulator created")

		inclinometer = InclinometerSimulator(config['inclinometer'])
		logger.info("✅ InclinometerSimulator created")

		event_manager = EventManager(config['simulation'])
		logger.info("✅ EventManager created")

		logger.info("🎉 All components initialized successfully!")

	except Exception as e:
		logger.error(f"❌ Component test failed: {e}")
		import traceback
		logger.error(f"Traceback: {traceback.format_exc()}")
		return False

	return True

if __name__ == "__main__":
	asyncio.run(test_components())
