#!/usr/bin/env python3
"""
Test script for Sensor Hub components
Verifies that all modules can be imported and initialized
"""

import asyncio
import sys
import os
import yaml

# Add src to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'src'))

async def test_components():
	"""Test all sensor hub components"""
	print("Testing Sensor Hub Components...")

	try:
		# Test database manager
		print("\n1. Testing Database Manager...")
		from database.database import DatabaseManager

		db = DatabaseManager(":memory:")  # Use in-memory database for testing
		await db.initialize()
		print("✓ Database Manager: OK")

		# Test cooling manager
		print("\n2. Testing Cooling Manager...")
		from sensors.cooling_manager import CoolingManager

		cooling_config = {
			'enabled': True,
			'low_speed_trigger_temp': 93,
			'high_speed_trigger_temp': 105,
			'low_speed_pin': 5,
			'high_speed_pin': 6,
			'temp_sensor_pin': 17,
			'temp_sensor_type': 'DS18B20'
		}

		cooling = CoolingManager(cooling_config)
		print("✓ Cooling Manager: OK")

		# Test TPMS manager
		print("\n3. Testing TPMS Manager...")
		from sensors.tpms_manager import TPMSManager

		tpms_config = {
			'enabled': True,
			'spi_bus': 0,
			'spi_device': 0,
			'cs_pin': 8,
			'gdo0_pin': 25,
			'gdo2_pin': 26,
			'sensors': []
		}

		tpms = TPMSManager(tpms_config)
		print("✓ TPMS Manager: OK")

		# Test power manager
		print("\n4. Testing Power Manager...")
		from power_management.power_manager import PowerManager

		power_config = {
			'enabled': True,
			'power_source_switch_pin': 18,
			'engine_signal_pin': 23,
			'system_led_pin': 24,
			'low_battery_threshold': 11.5,
			'critical_battery_threshold': 10.5
		}

		power = PowerManager(power_config)
		print("✓ Power Manager: OK")

		# Test configuration loading
		print("\n5. Testing Configuration Loading...")
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

		# Test component integration
		print("\n6. Testing Component Integration...")

		# Test cooling manager methods
		status = cooling.get_status()
		print(f"  - Cooling status: {status['temperature_status']}")

		# Test TPMS manager methods
		data = tpms.get_data()
		print(f"  - TPMS sensors: {len(data)}")

		# Test power manager methods
		power_status = power.get_status()
		print(f"  - Power status: {power_status['power_source']}")

		print("\n✓ All components tested successfully!")
		return True

	except ImportError as e:
		print(f"❌ Import error: {e}")
		print("Make sure all required modules are available")
		return False
	except Exception as e:
		print(f"❌ Test error: {e}")
		return False

async def test_environmental_sensors():
	"""Test environmental sensor components"""
	print("\n=== Testing Environmental Sensors ===")

	try:
		# Test DHT sensor manager
		print("\n1. Testing DHT Sensor Manager...")
		from sensors.environmental_manager import EnvironmentalManager

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

		env_manager = EnvironmentalManager(env_config)
		print("✓ Environmental Manager: OK")

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
		print("✓ Environmental sensors test completed")

		return True

	except ImportError as e:
		print(f"⚠ Environmental sensors not available: {e}")
		return False
	except Exception as e:
		print(f"❌ Environmental sensor test error: {e}")
		return False

async def main():
	"""Main test function"""
	print("=== Sensor Hub Component Test Suite ===\n")

	# Test core components
	components_ok = await test_components()

	# Test environmental sensors
	env_ok = await test_environmental_sensors()

	if components_ok and env_ok:
		print("\n🎉 All tests passed! Sensor Hub is ready.")
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
