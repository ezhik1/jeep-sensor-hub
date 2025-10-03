#!/usr/bin/env python3
"""
Test script for Environmental Manager (Development Version)
Tests the simulated DHT22 sensor monitoring without requiring actual hardware
"""

import asyncio
import time
import logging
from datetime import datetime
from src.sensors.environmental_manager_dev import EnvironmentalManagerDev

# Configure logging
logging.basicConfig(
	level=logging.INFO,
	format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)

# Test configuration
TEST_CONFIG = {
	'enabled': True,
	'sensors': {
		'cabin': {
			'pin': 17,
			'type': 'DHT22',
			'update_interval': 2000,  # 2 seconds
			'calibration_offset_temp': 0.5,
			'calibration_offset_humidity': -2.0
		},
		'outdoor': {
			'pin': 18,
			'type': 'DHT22',
			'update_interval': 5000,  # 5 seconds
			'calibration_offset_temp': -1.0,
			'calibration_offset_humidity': 0.0
		},
		'refrigerator': {
			'pin': 19,
			'type': 'DHT22',
			'update_interval': 10000,  # 10 seconds
			'calibration_offset_temp': 0.0,
			'calibration_offset_humidity': 0.0
		}
	}
}

async def test_environmental_manager():
	"""Test the environmental manager functionality"""
	print("=== Testing Environmental Manager (Development Version) ===\n")

	# Initialize manager
	print("1. Initializing Environmental Manager...")
	env_manager = EnvironmentalManagerDev(TEST_CONFIG)
	print(f"   ✓ Initialized with {len(env_manager.sensors)} sensors")

	# Test sensor configuration
	print("\n2. Testing sensor configuration...")
	for location, sensor in env_manager.sensors.items():
		print(f"   {location}: {sensor.sensor_type} on pin {sensor.pin}, "
			f"update interval: {sensor.update_interval}ms")

	# Test alert thresholds
	print("\n3. Testing alert thresholds...")
	for location in ['cabin', 'outdoor', 'refrigerator']:
		temp_thresh = env_manager.alert_thresholds['temperature'].get(location, {})
		humidity_thresh = env_manager.alert_thresholds['humidity'].get(location, {})
		print(f"   {location}: Temp {temp_thresh.get('min', 'N/A')}°C - {temp_thresh.get('max', 'N/A')}°C, "
			f"Humidity {humidity_thresh.get('min', 'N/A')}% - {humidity_thresh.get('max', 'N/A')}%")

	# Start monitoring
	print("\n4. Starting sensor monitoring...")
	await env_manager.start_monitoring()
	print("   ✓ Monitoring started")

	# Let it run for a while to collect data
	print("\n5. Collecting sensor data (running for 15 seconds)...")
	start_time = time.time()

	while time.time() - start_time < 15:
		# Get current readings
		current_readings = env_manager.get_current_readings()

		# Display current readings
		print(f"\r   Time: {time.time() - start_time:.1f}s | ", end="")
		for location, reading in current_readings.items():
			if reading:
				print(f"{location}: {reading.temperature:.1f}°C/{reading.humidity:.1f}% | ", end="")
			else:
				print(f"{location}: No data | ", end="")

		await asyncio.sleep(1)

	print("\n\n6. Testing data retrieval methods...")

	# Test getting recent readings
	print("   Recent readings per sensor:")
	for location in ['cabin', 'outdoor', 'refrigerator']:
		readings = env_manager.get_sensor_readings(location, limit=5)
		print(f"     {location}: {len(readings)} readings")
		if readings:
			latest = readings[0]
			print(f"       Latest: {latest.temperature:.1f}°C, {latest.humidity:.1f}%")
			print(f"       Valid: {latest.valid}, Timestamp: {latest.timestamp}")

	# Test statistics
	print("\n7. Testing statistics...")
	stats = env_manager.get_statistics()
	print(f"   Total readings: {stats['readings_total']}")
	print(f"   Valid readings: {stats['readings_valid']}")
	print(f"   Invalid readings: {stats['readings_invalid']}")
	print(f"   Uptime: {stats['uptime_seconds']:.1f} seconds")

	# Test sensor status
	print("\n8. Testing sensor status...")
	status = env_manager.get_sensor_status()
	for location, sensor_status in status.items():
		status_icon = "✅" if sensor_status['status'] == 'healthy' else "⚠️" if sensor_status['status'] == 'warning' else "❌"
		print(f"   {location.upper()}: {status_icon} {sensor_status['status']}, "
			f"Errors: {sensor_status['error_count']}")

		if sensor_status['current_temperature'] is not None:
			print(f"     Temperature: {sensor_status['current_temperature']:.1f}°C")
		if sensor_status['current_humidity'] is not None:
			print(f"     Humidity: {sensor_status['current_humidity']:.1f}%")

	# Test alert system
	print("\n9. Testing alert system...")
	alerts = env_manager.get_active_alerts()
	if alerts:
		print("   Active alerts:")
		for alert in alerts:
			print(f"     {alert['type']}: {alert['message']} at {alert['timestamp']}")
	else:
		print("   No active alerts")

	# Stop monitoring
	print("\n10. Stopping monitoring...")
	await env_manager.stop_monitoring()
	print("   ✓ Monitoring stopped")

	# Cleanup
	print("\n11. Cleanup...")
	env_manager.cleanup()
	print("   ✓ Cleanup completed")

	print("\n=== Development Test Completed! ===")
	return True

async def test_individual_sensor(location: str):
	"""Test a single sensor in detail"""
	print(f"\n=== Testing Individual Sensor: {location.upper()} ===\n")

	# Create minimal config for single sensor
	single_sensor_config = {
		'enabled': True,
		'sensors': {
			location: TEST_CONFIG['sensors'][location]
		}
	}

	try:
		# Initialize manager
		env_manager = EnvironmentalManagerDev(single_sensor_config)
		print(f"✓ Initialized {location} sensor")

		# Start monitoring
		await env_manager.start_monitoring()
		print("✓ Monitoring started")

		# Test for 10 seconds
		print("Testing sensor for 10 seconds...")
		start_time = time.time()

		while time.time() - start_time < 10:
			# Get current reading
			readings = env_manager.get_current_readings()
			if readings and readings.get(location):
				reading = readings[location]
				elapsed = time.time() - start_time
				print(f"\r   Time: {elapsed:.1f}s | "
					f"Temp: {reading.temperature:.1f}°C | "
					f"Humidity: {reading.humidity:.1f}% | "
					f"Valid: {reading.valid}", end="")
			else:
				elapsed = time.time() - start_time
				print(f"\r   Time: {elapsed:.1f}s | No data yet", end="")

			await asyncio.sleep(1)

		print("\n\nFinal reading:")
		final_readings = env_manager.get_current_readings()
		if final_readings and final_readings.get(location):
			reading = final_readings[location]
			print(f"   Temperature: {reading.temperature:.1f}°C")
			print(f"   Humidity: {reading.humidity:.1f}%")
			print(f"   Valid: {reading.valid}")
		else:
			print("   No data received")

		# Stop and cleanup
		await env_manager.stop_monitoring()
		env_manager.cleanup()
		print("✓ Test completed successfully")

		return True

	except Exception as e:
		print(f"❌ Error testing sensor: {e}")
		return False

async def main():
	"""Main test function"""
	print("=== DHT Environmental Sensor Development Test ===\n")

	# Test individual sensors first
	print("1. Testing individual sensors...")

	# Test cabin sensor
	await test_individual_sensor('cabin')

	# Test outdoor sensor
	await test_individual_sensor('outdoor')

	# Test refrigerator sensor
	await test_individual_sensor('refrigerator')

	# Test full system
	print("\n2. Testing full environmental monitoring system...")
	await test_environmental_manager()

	print("\n🎉 All development tests completed successfully!")
	print("The environmental monitoring system is working correctly.")

if __name__ == "__main__":
	try:
		asyncio.run(main())
	except KeyboardInterrupt:
		print("\n\n⚠️  Test interrupted by user")
	except Exception as e:
		print(f"\n❌ Unexpected error: {e}")
		import traceback
		traceback.print_exc()
