#!/usr/bin/env python3
"""
Event Manager for Mock Sensor Hub
Manages simulation events, route tracking, and engine state changes
"""

import random
import logging
from datetime import datetime, timedelta
from typing import Dict, List, Optional, Any, Callable

logger = logging.getLogger(__name__)

class EventManager:
	def __init__(self, config: Dict):
		self.config = config
		self.enabled = config.get('enabled', True)

		# Event configuration
		self.events = config.get('events', [])
		self.routes = config.get('routes', [])
		self.engine_events = []

		# Event callbacks
		self.event_callbacks = {}

		# Current state
		self.current_route = None
		self.route_start_time = None
		self.engine_running = False
		self.last_engine_change = datetime.now()

		# Statistics
		self.total_events = 0
		self.total_routes = 0
		self.total_engine_cycles = 0

		# Initialize default events if none configured
		self.initialize_default_events()

		# Event scheduling
		self.scheduled_events = []
		self.event_timers = {}

	def initialize_default_events(self):
		"""Initialize default simulation events if none configured"""
		if not self.events:
			self.events = [
				{
					'id': 'engine_start',
					'type': 'engine_state',
					'trigger': 'manual',
					'probability': 0.1,  # 10% chance per check
					'conditions': {'engine_running': False},
					'actions': [
						{'action': 'start_engine', 'delay': 0},
						{'action': 'start_route', 'delay': 2}
					],
					'description': 'Engine Start Event'
				},
				{
					'id': 'engine_stop',
					'type': 'engine_state',
					'trigger': 'manual',
					'probability': 0.05,  # 5% chance per check
					'conditions': {'engine_running': True, 'route_distance': 'min_100m'},
					'actions': [
						{'action': 'stop_route', 'delay': 0},
						{'action': 'stop_engine', 'delay': 1}
					],
					'description': 'Engine Stop Event'
				},
				{
					'id': 'weather_change',
					'type': 'environmental',
					'trigger': 'time_based',
					'interval': 300,  # Every 5 minutes
					'conditions': {},
					'actions': [
						{'action': 'change_weather', 'delay': 0}
					],
					'description': 'Weather Change Event'
				},
				{
					'id': 'terrain_change',
					'type': 'movement',
					'trigger': 'route_based',
					'conditions': {'route_progress': '25%'},
					'actions': [
						{'action': 'change_terrain', 'delay': 0}
					],
					'description': 'Terrain Change Event'
				},
				{
					'id': 'sensor_failure',
					'type': 'system',
					'trigger': 'random',
					'probability': 0.001,  # 0.1% chance per check
					'conditions': {},
					'actions': [
						{'action': 'simulate_sensor_failure', 'delay': 0}
					],
					'description': 'Sensor Failure Simulation'
				}
			]

		if not self.routes:
			self.routes = [
				{
					'id': 'city_route',
					'name': 'City Driving Route',
					'description': 'Urban driving with traffic lights and stops',
					'waypoints': [
						{'latitude': 40.7128, 'longitude': -74.0060, 'name': 'Start'},
						{'latitude': 40.7589, 'longitude': -73.9851, 'name': 'Times Square'},
						{'latitude': 40.7505, 'longitude': -73.9934, 'name': 'Penn Station'},
						{'latitude': 40.7484, 'longitude': -73.9857, 'name': 'Madison Square Garden'},
						{'latitude': 40.7128, 'longitude': -74.0060, 'name': 'End'}
					],
					'speed_range': (5.0, 15.0),  # m/s
					'terrain': 'flat',
					'road_conditions': 'smooth',
					'traffic_density': 'high'
				},
				{
					'id': 'highway_route',
					'name': 'Highway Driving Route',
					'description': 'Highway driving with consistent speed',
					'waypoints': [
						{'latitude': 40.7128, 'longitude': -74.0060, 'name': 'Start'},
						{'latitude': 40.7589, 'longitude': -73.9851, 'name': 'Highway Entry'},
						{'latitude': 40.7505, 'longitude': -73.9934, 'name': 'Highway Exit'},
						{'latitude': 40.7484, 'longitude': -73.9857, 'name': 'End'}
					],
					'speed_range': (20.0, 30.0),  # m/s
					'terrain': 'flat',
					'road_conditions': 'smooth',
					'traffic_density': 'low'
				},
				{
					'id': 'offroad_route',
					'name': 'Off-road Adventure Route',
					'description': 'Off-road driving with varied terrain',
					'waypoints': [
						{'latitude': 40.7128, 'longitude': -74.0060, 'name': 'Start'},
						{'latitude': 40.7589, 'longitude': -73.9851, 'name': 'Trail Head'},
						{'latitude': 40.7505, 'longitude': -73.9934, 'name': 'Mountain Pass'},
						{'latitude': 40.7484, 'longitude': -73.9857, 'name': 'End'}
					],
					'speed_range': (2.0, 8.0),  # m/s
					'terrain': 'mountainous',
					'road_conditions': 'offroad',
					'traffic_density': 'none'
				}
			]

	def register_callback(self, event_type: str, callback: Callable):
		"""Register a callback function for a specific event type"""
		if event_type not in self.event_callbacks:
			self.event_callbacks[event_type] = []

		self.event_callbacks[event_type].append(callback)
		logger.info(f"Registered callback for event type: {event_type}")

	def unregister_callback(self, event_type: str, callback: Callable):
		"""Unregister a callback function"""
		if event_type in self.event_callbacks and callback in self.event_callbacks[event_type]:
			self.event_callbacks[event_type].remove(callback)
			logger.info(f"Unregistered callback for event type: {event_type}")

	async def process_events(self, current_state: Dict = None):
		"""Process all pending events"""
		if not self.enabled:
			return

		# Update current state
		if current_state:
			self._update_current_state(current_state)

		# Process scheduled events
		self._process_scheduled_events()

		# Check for new events
		self._check_event_triggers()

		# Update event timers
		self._update_event_timers()

	def _update_current_state(self, state: Dict):
		"""Update current system state"""
		if 'engine_running' in state:
			self.engine_running = state['engine_running']

		if 'current_route' in state:
			self.current_route = state['current_route']

		if 'route_start_time' in state:
			self.route_start_time = state['route_start_time']

	def _process_scheduled_events(self):
		"""Process events that are scheduled to occur"""
		current_time = datetime.now()
		events_to_execute = []

		for event in self.scheduled_events:
			if event['execute_time'] <= current_time:
				events_to_execute.append(event)

		for event in events_to_execute:
			self._execute_event(event)
			self.scheduled_events.remove(event)

	def _check_event_triggers(self):
		"""Check if any events should be triggered"""
		for event in self.events:
			if self._should_trigger_event(event):
				self._schedule_event(event)

	def _should_trigger_event(self, event: Dict) -> bool:
		"""Determine if an event should be triggered"""
		trigger_type = event.get('trigger', 'manual')

		if trigger_type == 'manual':
			return False  # Manual events are triggered by user

		elif trigger_type == 'probability':
			probability = event.get('probability', 0.0)
			return random.random() < probability

		elif trigger_type == 'time_based':
			interval = event.get('interval', 60)
			event_id = event['id']

			if event_id not in self.event_timers:
				self.event_timers[event_id] = datetime.now()
				return False

			time_since_last = (datetime.now() - self.event_timers[event_id]).total_seconds()
			if time_since_last >= interval:
				self.event_timers[event_id] = datetime.now()
				return True

		elif trigger_type == 'route_based':
			if not self.current_route:
				return False

			conditions = event.get('conditions', {})
			if 'route_progress' in conditions:
				progress = self._calculate_route_progress()
				if progress >= 0.25:  # 25% progress
					return True

		elif trigger_type == 'condition_based':
			conditions = event.get('conditions', {})
			return self._check_conditions(conditions)

		return False

	def _check_conditions(self, conditions: Dict) -> bool:
		"""Check if conditions are met for an event"""
		for condition, value in conditions.items():
			if condition == 'engine_running':
				if self.engine_running != value:
					return False
			elif condition == 'route_distance':
				if value == 'min_100m':
					if not self.current_route or self._calculate_route_distance() < 100:
						return False
			# Add more condition checks as needed

		return True

	def _schedule_event(self, event: Dict):
		"""Schedule an event for execution"""
		current_time = datetime.now()

		for action in event.get('actions', []):
			delay = action.get('delay', 0)
			execute_time = current_time + timedelta(seconds=delay)

			scheduled_event = {
				'event_id': event['id'],
				'event_type': event['type'],
				'action': action['action'],
				'execute_time': execute_time,
				'description': event.get('description', 'Unknown Event')
			}

			self.scheduled_events.append(scheduled_event)
			self.scheduled_events.sort(key=lambda x: x['execute_time'])

			logger.info(f"Scheduled event: {event['id']} - {action['action']} at {execute_time}")

	def _execute_event(self, event: Dict):
		"""Execute a scheduled event"""
		try:
			action = event['action']
			event_type = event['event_type']

			# Execute the action
			if action == 'start_engine':
				self._start_engine()
			elif action == 'stop_engine':
				self._stop_engine()
			elif action == 'start_route':
				self._start_route()
			elif action == 'stop_route':
				self._stop_route()
			elif action == 'change_weather':
				self._change_weather()
			elif action == 'change_terrain':
				self._change_terrain()
			elif action == 'simulate_sensor_failure':
				self._simulate_sensor_failure()

			# Notify callbacks
			self._notify_callbacks(event_type, event)

			# Update statistics
			self.total_events += 1

			logger.info(f"Executed event: {event['event_id']} - {action}")

		except Exception as e:
			logger.error(f"Error executing event {event['event_id']}: {e}")

	def _start_engine(self):
		"""Start the engine simulation"""
		if not self.engine_running:
			self.engine_running = True
			self.last_engine_change = datetime.now()
			self.total_engine_cycles += 1

			# Record engine event
			engine_event = {
				'timestamp': datetime.now().isoformat(),
				'event_type': 'engine_start',
				'metadata': {
					'cycle_number': self.total_engine_cycles,
					'previous_runtime': self._calculate_total_runtime()
				}
			}
			self.engine_events.append(engine_event)

	def _stop_engine(self):
		"""Stop the engine simulation"""
		if self.engine_running:
			self.engine_running = False
			self.last_engine_change = datetime.now()

			# Record engine event
			runtime = self._calculate_total_runtime()
			engine_event = {
				'timestamp': datetime.now().isoformat(),
				'event_type': 'engine_stop',
				'metadata': {
					'runtime_seconds': runtime,
					'total_cycles': self.total_engine_cycles
				}
			}
			self.engine_events.append(engine_event)

	def _start_route(self):
		"""Start a new route"""
		if not self.current_route:
			# Select a random route
			if self.routes:
				route = random.choice(self.routes)
				self.current_route = route
				self.route_start_time = datetime.now()
				self.total_routes += 1

				logger.info(f"Started route: {route['name']}")

	def _stop_route(self):
		"""Stop the current route"""
		if self.current_route:
			route_name = self.current_route['name']
			self.current_route = None
			self.route_start_time = None

			logger.info(f"Stopped route: {route_name}")

	def _change_weather(self):
		"""Change weather conditions"""
		weather_options = ['clear', 'cloudy', 'rainy', 'foggy']
		new_weather = random.choice(weather_options)

		# This would typically update the environmental simulator
		logger.info(f"Weather changed to: {new_weather}")

	def _change_terrain(self):
		"""Change terrain type"""
		terrain_options = ['flat', 'hilly', 'mountainous']
		new_terrain = random.choice(terrain_options)

		# This would typically update the inclinometer simulator
		logger.info(f"Terrain changed to: {new_terrain}")

	def _simulate_sensor_failure(self):
		"""Simulate a sensor failure"""
		failure_types = ['noise_increase', 'drift_increase', 'calibration_error', 'communication_loss']
		failure_type = random.choice(failure_types)

		# This would typically update the affected sensor simulator
		logger.info(f"Simulated sensor failure: {failure_type}")

	def _notify_callbacks(self, event_type: str, event: Dict):
		"""Notify registered callbacks of an event"""
		if event_type in self.event_callbacks:
			for callback in self.event_callbacks[event_type]:
				try:
					callback(event)
				except Exception as e:
					logger.error(f"Error in event callback: {e}")

	def _update_event_timers(self):
		"""Update event timers"""
		current_time = datetime.now()

		# Clean up old engine events (keep last 100)
		if len(self.engine_events) > 100:
			self.engine_events = self.engine_events[-100:]

	def _calculate_route_progress(self) -> float:
		"""Calculate current route progress (0.0 to 1.0)"""
		if not self.current_route or not self.route_start_time:
			return 0.0

		# Simple time-based progress for now
		elapsed = (datetime.now() - self.route_start_time).total_seconds()
		estimated_duration = 600  # 10 minutes default
		return min(1.0, elapsed / estimated_duration)

	def _calculate_route_distance(self) -> float:
		"""Calculate current route distance in meters"""
		if not self.current_route:
			return 0.0

		# Simple estimation based on time and speed
		if not self.route_start_time:
			return 0.0

		elapsed = (datetime.now() - self.route_start_time).total_seconds()
		avg_speed = 10.0  # m/s default
		return elapsed * avg_speed

	def _calculate_total_runtime(self) -> float:
		"""Calculate total engine runtime in seconds"""
		if not self.engine_running:
			return 0.0

		return (datetime.now() - self.last_engine_change).total_seconds()

	def trigger_manual_event(self, event_id: str):
		"""Manually trigger an event"""
		event = next((e for e in self.events if e['id'] == event_id), None)
		if event:
			self._schedule_event(event)
			logger.info(f"Manually triggered event: {event_id}")
		else:
			logger.warning(f"Manual event not found: {event_id}")

	def get_event_summary(self) -> Dict:
		"""Get summary of all events and routes"""
		return {
			'enabled': self.enabled,
			'total_events': self.total_events,
			'total_routes': self.total_routes,
			'total_engine_cycles': self.total_engine_cycles,
			'current_state': {
				'engine_running': self.engine_running,
				'current_route': self.current_route['name'] if self.current_route else None,
				'route_progress': self._calculate_route_progress()
			},
			'available_events': [e['id'] for e in self.events],
			'available_routes': [r['id'] for r in self.routes],
			'scheduled_events': len(self.scheduled_events),
			'engine_events': len(self.engine_events)
		}

	def add_event(self, event_config: Dict):
		"""Add a new simulation event"""
		if 'id' not in event_config:
			logger.error("Event config must include 'id' field")
			return

		# Check if event already exists
		if any(e['id'] == event_config['id'] for e in self.events):
			logger.warning(f"Event {event_config['id']} already exists")
			return

		self.events.append(event_config)
		logger.info(f"Added new event: {event_config['id']}")

	def remove_event(self, event_id: str):
		"""Remove a simulation event"""
		self.events = [e for e in self.events if e['id'] != event_id]
		logger.info(f"Removed event: {event_id}")

	def add_route(self, route_config: Dict):
		"""Add a new route"""
		if 'id' not in route_config:
			logger.error("Route config must include 'id' field")
			return

		# Check if route already exists
		if any(r['id'] == route_config['id'] for r in self.routes):
			logger.warning(f"Route {route_config['id']} already exists")
			return

		self.routes.append(route_config)
		logger.info(f"Added new route: {route_config['id']}")

	def remove_route(self, route_id: str):
		"""Remove a route"""
		self.routes = [r for r in self.routes if r['id'] != route_id]
		logger.info(f"Removed route: {route_id}")

	def reset_statistics(self):
		"""Reset all statistics"""
		self.total_events = 0
		self.total_routes = 0
		self.total_engine_cycles = 0
		self.engine_events.clear()
		logger.info("Event manager statistics reset")

	async def get_data(self) -> Dict:
		"""Get current event manager data for API responses"""
		if not self.enabled:
			return {
				'enabled': False,
				'error': 'Event manager disabled'
			}

		return {
			'enabled': True,
			'summary': self.get_event_summary(),
			'events': self.events,
			'routes': self.routes,
			'scheduled_events': self.scheduled_events,
			'engine_events': self.engine_events[-10:],  # Last 10 events
			'timestamp': datetime.now().isoformat()
		}
