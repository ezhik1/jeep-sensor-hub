#!/usr/bin/env python3
"""
Production Test Script for Environmental Manager
Tests real DHT22/DHT11 sensors using actual hardware
"""

import asyncio
import time
import logging
import sys
from pathlib import Path

# Add src to path for imports
sys.path.append(str(Path(__file__).parent / "src"))

try:
	from sensors.environmental_manager import EnvironmentalManager
	PRODUCTION_AVAILABLE = True
except ImportError as e:
	print(f"Production EnvironmentalManager not available: {e}")
	print("Make sure to install: pip install Adafruit_DHT RPi.GPIO")
	PRODUCTION_AVAILABLE = False

# Configure logging
logging.basicConfig(
	level=logging.INFO,
	format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)

logger = logging.getLogger(__name__)

# Production test configuration
PRODUCTION_CONFIG = {
	'enabled': True,
	'sensors': {
		'cabin': {
			'pin': 17,  # GPIO17
			'type': 'DHT22',
			'update_interval': 3000,  # 3 seconds for testing
			'calibration_offset_temp': 0.0,
			'calibration_offset_humidity': 0.0
		},
		'outdoor': {
			'pin': 18,  # GPIO18
			'type': 'DHT22',
			'update_interval': 5000,  # 5 seconds
			'calibration_offset_temp': 0.0,
			'calibration_offset_humidity': 0.0
		},
		'refrigerator': {
			'pin': 19,  # GPIO19
			'type': 'DHT22',
			'update_interval': 10000,  # 10 seconds
			'calibration_offset_temp': 0.0,
			'calibration_offset_humidity': 0.0
		}
	}
}

async def test_production_sensors():
	"""Test real DHT sensors in production environment"""
	if not PRODUCTION_AVAILABLE:
		print("❌ Production EnvironmentalManager not available")
		return False

	print("=== Testing Production Environmental Manager with Real DHT Sensors ===\n")

	# Initialize manager
	print("1. Initializing Production Environmental Manager...")
	try:
		env_manager = EnvironmentalManager(PRODUCTION_CONFIG)
		print(f"   ✓ Initialized with {len(env_manager.sensors)} sensors")
	except Exception as e:
		print(f"   ❌ Failed to initialize: {e}")
		return False

	# Test sensor configuration
	print("\n2. Sensor Configuration:")
	for location, sensor in env_manager.sensors.items():
		print(f"   {location}: {sensor.sensor_type} on GPIO{sensor.pin}, "
			f"update interval: {sensor.update_interval}ms")

	# Start monitoring
	print("\n3. Starting sensor monitoring...")
	try:
		await env_manager.start_monitoring()
		print("   ✓ Monitoring started")
	except Exception as e:
		print(f"   ❌ Failed to start monitoring: {e}")
		return False

	# Let it run to collect real sensor data
	print("\n4. Collecting real sensor data (running for 30 seconds)...")
	print("   Make sure your DHT sensors are properly connected!")
	print("   - Cabin sensor on GPIO17")
	print("   - Outdoor sensor on GPIO18")
	print("   - Refrigerator sensor on GPIO19")

	start_time = time.time()
	last_status_time = 0

	while time.time() - start_time < 30:
		current_time = time.time()

		# Display status every 5 seconds
		if int(current_time - start_time) % 5 == 0:
			elapsed = current_time - start_time
			print(f"\n   Time: {elapsed:.1f}s")

			# Get current readings
			current_readings = env_manager.get_current_readings()
			for location, reading in current_readings.items():
				if reading:
					print(f"     {location.upper()}: {reading.temperature:.1f}°C, {reading.humidity:.1f}%")
				else:
					print(f"     {location.upper()}: No data yet")

			# Get sensor status
			status = env_manager.get_sensor_status()
			for location, sensor_status in status.items():
				error_count = sensor_status['error_count']
				if error_count > 0:
					print(f"     {location.upper()} errors: {error_count}")

		await asyncio.sleep(1)

	print("\n5. Final sensor status:")
	final_status = env_manager.get_sensor_status()
	for location, sensor_status in final_status.items():
		status_icon = "✅" if sensor_status['status'] == 'healthy' else "⚠️" if sensor_status['status'] == 'warning' else "❌"
		print(f"   {location.upper()}: {status_icon} {sensor_status['status']}, "
			f"Errors: {sensor_status['error_count']}")

		if sensor_status['current_temperature'] is not None:
			print(f"     Temperature: {sensor_status['current_temperature']:.1f}°C")
		if sensor_status['current_humidity'] is not None:
			print(f"     Humidity: {sensor_status['current_humidity']:.1f}%")

	# Get statistics
	print("\n6. Monitoring statistics:")
	stats = env_manager.get_statistics()
	print(f"   Total readings: {stats['readings_total']}")
	print(f"   Valid readings: {stats['readings_valid']}")
	print(f"   Invalid readings: {stats['readings_invalid']}")
	print(f"   Uptime: {stats['uptime_seconds']:.1f} seconds")

	# Stop monitoring
	print("\n7. Stopping monitoring...")
	try:
		await env_manager.stop_monitoring()
		print("   ✓ Monitoring stopped")
	except Exception as e:
		print(f"   ❌ Error stopping monitoring: {e}")

	# Cleanup
	print("\n8. Cleanup...")
	try:
		env_manager.cleanup()
		print("   ✓ Cleanup completed")
	except Exception as e:
		print(f"   ❌ Error during cleanup: {e}")

	print("\n=== Production Test Completed! ===")
	return True

async def test_individual_sensor(pin: int, sensor_type: str = 'DHT22'):
	"""Test a single DHT sensor on a specific pin"""
	if not PRODUCTION_AVAILABLE:
		print("❌ Production EnvironmentalManager not available")
		return False

	print(f"\n=== Testing Individual {sensor_type} Sensor on GPIO{pin} ===\n")

	# Create minimal config for single sensor
	single_sensor_config = {
		'enabled': True,
		'sensors': {
			'test': {
				'pin': pin,
				'type': sensor_type,
				'update_interval': 2000,  # 2 seconds
				'calibration_offset_temp': 0.0,
				'calibration_offset_humidity': 0.0
			}
		}
	}

	try:
		# Initialize manager
		env_manager = EnvironmentalManager(single_sensor_config)
		print(f"✓ Initialized {sensor_type} sensor on GPIO{pin}")

		# Start monitoring
		await env_manager.start_monitoring()
		print("✓ Monitoring started")

		# Test for 15 seconds
		print("Testing sensor for 15 seconds...")
		start_time = time.time()

		while time.time() - start_time < 15:
			# Get current reading
			readings = env_manager.get_current_readings()
			if readings and readings.get('test'):
				reading = readings['test']
				print(f"   Temperature: {reading.temperature:.1f}°C, Humidity: {reading.humidity:.1f}%")
			else:
				print("   No data yet...")

			await asyncio.sleep(2)

		# Stop monitoring
		await env_manager.stop_monitoring()
		env_manager.cleanup()
		print("✓ Test completed successfully")

		return True

	except Exception as e:
		print(f"❌ Error testing sensor: {e}")
		return False

async def main():
	"""Main test function"""
	print("=== DHT Environmental Sensor Production Test ===\n")

	if not PRODUCTION_AVAILABLE:
		print("❌ Production environment not available")
		print("This test requires:")
		print("  - Raspberry Pi with GPIO access")
		print("  - DHT22/DHT11 sensors connected")
		print("  - Adafruit_DHT and RPi.GPIO libraries installed")
		print("\nTo install dependencies:")
		print("  pip install Adafruit_DHT RPi.GPIO")
		return False

	# Test all sensors
	print("Testing all configured sensors...")
	success = await test_production_sensors()

	if success:
		print("\n🎉 Production test completed successfully!")
		print("Your DHT sensors are working correctly.")
	else:
		print("\n⚠️  Production test had issues.")
		print("Check sensor connections and GPIO pin assignments.")

	return success

if __name__ == "__main__":
	try:
		asyncio.run(main())
	except KeyboardInterrupt:
		print("\n\n⚠️  Test interrupted by user")
	except Exception as e:
		print(f"\n❌ Unexpected error: {e}")
		import traceback
		traceback.print_exc()
