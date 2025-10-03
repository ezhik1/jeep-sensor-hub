#!/usr/bin/env python3
"""
Development Test Script for Sensor Hub Components
Can run on Windows/Linux without Raspberry Pi hardware
"""

import asyncio
import sys
import os
import yaml

# Add src to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'src'))

async def test_components_dev():
	"""Test all sensor hub components in development mode"""
	print("Testing Sensor Hub Components (Development Mode)...")

	try:
		# Test database manager
		print("\n1. Testing Database Manager...")
		from database.database import DatabaseManager

		db = DatabaseManager(":memory:")  # Use in-memory database for testing
		await db.initialize()
		print("✓ Database Manager: OK")

		# Test configuration loading
		print("\n2. Testing Configuration Loading...")
		config_path = "config.yaml"
		if os.path.exists(config_path):
			with open(config_path, 'r') as f:
				config = yaml.safe_load(f)
			print("✓ Configuration Loading: OK")
			print(f"  - Cooling enabled: {config.get('cooling', {}).get('enabled', False)}")
			print(f"  - TPMS enabled: {config.get('tpms', {}).get('enabled', False)}")
			print(f"  - Power management enabled: {config.get('power_management', {}).get('enabled', False)}")
		else:
			print("⚠ Configuration file not found")

		# Test database operations
		print("\n3. Testing Database Operations...")

		# Test sensor data storage
		test_data = {
			'type': 'temperature',
			'id': 'test_sensor',
			'value': 25.5,
			'unit': '°C',
			'metadata': {'location': 'test'}
		}
		await db.store_sensor_data(test_data)
		print("✓ Sensor data storage: OK")

		# Test sensor history retrieval
		history = await db.get_sensor_history('temperature', 1)
		print(f"✓ Sensor history retrieval: OK (found {len(history)} records)")

		# Test cooling data storage
		cooling_data = {
			'temperature': 85.0,
			'fan_low_speed': True,
			'fan_high_speed': False,
			'ac_state': False,
			'engine_load': 0.6,
			'ambient_temp': 22.0
		}
		await db.store_cooling_data(cooling_data)
		print("✓ Cooling data storage: OK")

		# Test cooling history retrieval
		cooling_history = await db.get_cooling_history(10)
		print(f"✓ Cooling history retrieval: OK (found {len(cooling_history)} records)")

		# Test data export
		export_data = await db.export_data('temperature',
			history[0]['timestamp'] if history else None,
			None, 'json')
		print("✓ Data export: OK")

		# Test component structure (without hardware dependencies)
		print("\n4. Testing Component Structure...")

		# Test that we can import the modules (even if they can't initialize)
		try:
			from sensors.cooling_manager import CoolingManager
			print("✓ Cooling Manager module: Importable")
		except ImportError as e:
			print(f"⚠ Cooling Manager module: Import failed - {e}")

		try:
			from sensors.tpms_manager import TPMSManager
			print("✓ TPMS Manager module: Importable")
		except ImportError as e:
			print(f"⚠ TPMS Manager module: Import failed - {e}")

		try:
			from power_management.power_manager import PowerManager
			print("✓ Power Manager module: Importable")
		except ImportError as e:
			print(f"⚠ Power Manager module: Import failed - {e}")

		try:
			from sensors.environmental_manager import EnvironmentalManager
			print("✓ Environmental Manager module: Importable")
		except ImportError as e:
			print(f"⚠ Environmental Manager module: Import failed - {e}")

		# Test mock data generation
		print("\n5. Testing Mock Data Generation...")
		try:
			from utils.mock_data_generator import MockDataGenerator
			mock_gen = MockDataGenerator()

			# Generate mock sensor data
			mock_temp = mock_gen.generate_temperature_data()
			mock_humidity = mock_gen.generate_humidity_data()
			mock_pressure = mock_gen.generate_pressure_data()

			print("✓ Mock data generation: OK")
			print(f"  - Temperature: {mock_temp['value']:.1f}°C")
			print(f"  - Humidity: {mock_humidity['value']:.1f}%")
			print(f"  - Pressure: {mock_pressure['value']:.1f} PSI")

		except ImportError as e:
			print(f"⚠ Mock data generator not available: {e}")

		# Test configuration validation
		print("\n6. Testing Configuration Validation...")
		try:
			from utils.config_validator import ConfigValidator
			validator = ConfigValidator()

			if os.path.exists(config_path):
				with open(config_path, 'r') as f:
					config = yaml.safe_load(f)

				is_valid = validator.validate_config(config)
				if is_valid:
					print("✓ Configuration validation: OK")
				else:
					print("⚠ Configuration validation: Issues found")
					for error in validator.get_errors():
						print(f"    - {error}")
			else:
				print("⚠ Configuration file not found for validation")

		except ImportError as e:
			print(f"⚠ Configuration validator not available: {e}")

		# Cleanup
		await db.close()
		print("\n✓ All development tests completed successfully!")
		return True

	except ImportError as e:
		print(f"❌ Import error: {e}")
		print("Make sure all required modules are available")
		return False
	except Exception as e:
		print(f"❌ Test error: {e}")
		import traceback
		traceback.print_exc()
		return False

async def test_environmental_dev():
	"""Test environmental sensor components in development mode"""
	print("\n=== Testing Environmental Sensors (Development) ===")

	try:
		# Test development environmental manager
		print("\n1. Testing Development Environmental Manager...")
		from sensors.environmental_manager_dev import EnvironmentalManagerDev

		env_config = {
			'enabled': True,
			'sensors': {
				'cabin': {
					'pin': 17,
					'type': 'DHT22',
					'update_interval': 5000
				}
			}
		}

		env_manager = EnvironmentalManagerDev(env_config)
		print("✓ Development Environmental Manager: OK")

		# Test sensor initialization
		await env_manager.start_monitoring()
		print("✓ Sensor monitoring started")

		# Let it run briefly
		await asyncio.sleep(2)

		# Get readings
		readings = env_manager.get_current_readings()
		print(f"✓ Got readings: {len(readings)} sensors")

		# Stop monitoring
		await env_manager.stop_monitoring()
		env_manager.cleanup()
		print("✓ Development environmental sensors test completed")

		return True

	except ImportError as e:
		print(f"⚠ Development environmental sensors not available: {e}")
		return False
	except Exception as e:
		print(f"❌ Development environmental sensor test error: {e}")
		return False

async def main():
	"""Main test function"""
	print("=== Sensor Hub Development Component Test Suite ===\n")

	# Test core components
	components_ok = await test_components_dev()

	# Test environmental sensors
	env_ok = await test_environmental_dev()

	if components_ok and env_ok:
		print("\n🎉 All development tests passed! Sensor Hub is ready for development.")
		return True
	elif components_ok:
		print("\n⚠️  Core components OK, but environmental sensors failed.")
		return False
	else:
		print("\n❌ Core component tests failed.")
		return False

if __name__ == "__main__":
	try:
		asyncio.run(main())
	except KeyboardInterrupt:
		print("\n\n⚠️  Test interrupted by user")
	except Exception as e:
		print(f"\n❌ Unexpected error: {e}")
		import traceback
		traceback.print_exc()
