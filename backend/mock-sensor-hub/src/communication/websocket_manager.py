#!/usr/bin/env python3
"""
WebSocket Manager for Mock Sensor Hub
Handles WebSocket connections for real-time sensor data broadcasting
"""

import asyncio
import json
import logging
from datetime import datetime
from typing import Dict, List, Optional, Any, Set
from fastapi import WebSocket, WebSocketDisconnect

logger = logging.getLogger(__name__)

class WebSocketManager:
	def __init__(self):
		self.active_connections: Set[WebSocket] = set()
		self.connection_info: Dict[WebSocket, Dict] = {}
		self.broadcast_queue: asyncio.Queue = asyncio.Queue()
		self.running = False

		# Statistics
		self.total_connections = 0
		self.messages_sent = 0
		self.messages_received = 0

	async def connect(self, websocket: WebSocket, client_info: Dict = None):
		"""Accept a new WebSocket connection"""
		try:
		await websocket.accept()
		self.active_connections.add(websocket)

		# Store connection information
		connection_id = id(websocket)
		self.connection_info[websocket] = {
		'id': connection_id,
		'connected_at': datetime.now().isoformat(),
		'client_info': client_info or {},
		'last_activity': datetime.now(),
		'message_count': 0
		}

		self.total_connections += 1

		# Send welcome message
		welcome_msg = {
		'type': 'connection_established',
		'timestamp': datetime.now().isoformat(),
		'connection_id': connection_id,
		'message': 'Connected to Mock Sensor Hub'
		}
		await websocket.send_text(json.dumps(welcome_msg))

		logger.info(f"WebSocket connected: {connection_id} - Total connections: {len(self.active_connections)}")

		except Exception as e:
		logger.error(f"Error accepting WebSocket connection: {e}")
		if websocket in self.active_connections:
		self.active_connections.remove(websocket)

	async def disconnect(self, websocket: WebSocket):
		"""Handle WebSocket disconnection"""
		try:
		if websocket in self.active_connections:
		self.active_connections.remove(websocket)

		# Get connection info before removing
		connection_info = self.connection_info.get(websocket, {})
		connection_id = connection_info.get('id', 'unknown')

		# Remove connection info
		if websocket in self.connection_info:
		del self.connection_info[websocket]

		logger.info(f"WebSocket disconnected: {connection_id} - Remaining connections: {len(self.active_connections)}")

		except Exception as e:
		logger.error(f"Error during WebSocket disconnection: {e}")

	async def send_personal_message(self, message: Dict, websocket: WebSocket):
		"""Send a message to a specific WebSocket connection"""
		try:
		if websocket in self.active_connections:
		message_json = json.dumps(message)
		await websocket.send_text(message_json)

		# Update connection info
		if websocket in self.connection_info:
		self.connection_info[websocket]['message_count'] += 1
		self.connection_info[websocket]['last_activity'] = datetime.now()

		self.messages_sent += 1

		except Exception as e:
		logger.error(f"Error sending personal message: {e}")
		# Mark connection for removal
		await self.disconnect(websocket)

	async def broadcast(self, message: Dict):
		"""Broadcast a message to all connected WebSockets"""
		if not self.active_connections:
		return

		# Add timestamp if not present
		if 'timestamp' not in message:
		message['timestamp'] = datetime.now().isoformat()

		message_json = json.dumps(message)
		disconnected_websockets = []

		for websocket in self.active_connections:
		try:
		await websocket.send_text(message_json)

		# Update connection info
		if websocket in self.connection_info:
		self.connection_info[websocket]['message_count'] += 1
		self.connection_info[websocket]['last_activity'] = datetime.now()

		self.messages_sent += 1

		except Exception as e:
		logger.error(f"Error broadcasting message: {e}")
		disconnected_websockets.append(websocket)

		# Remove disconnected websockets
		for websocket in disconnected_websockets:
		await self.disconnect(websocket)

	async def broadcast_sensor_data(self, sensor_data: Dict):
		"""Broadcast sensor data to all connected clients"""
		message = {
		'type': 'sensor_data',
		'data': sensor_data
		}
		await self.broadcast(message)

	async def broadcast_system_status(self, status: Dict):
		"""Broadcast system status to all connected clients"""
		message = {
		'type': 'system_status',
		'data': status
		}
		await self.broadcast(message)

	async def broadcast_event(self, event: Dict):
		"""Broadcast an event to all connected clients"""
		message = {
		'type': 'event',
		'data': event
		}
		await self.broadcast(message)

	async def broadcast_alert(self, alert: Dict):
		"""Broadcast an alert to all connected clients"""
		message = {
		'type': 'alert',
		'data': alert
		}
		await self.broadcast(message)

	async def handle_message(self, websocket: WebSocket, message: str):
		"""Handle incoming WebSocket messages"""
		try:
		# Parse message
		data = json.loads(message)
		message_type = data.get('type', 'unknown')

		# Update connection info
		if websocket in self.connection_info:
		self.connection_info[websocket]['message_count'] += 1
		self.connection_info[websocket]['last_activity'] = datetime.now()

		self.messages_received += 1

		# Handle different message types
		if message_type == 'ping':
		await self._handle_ping(websocket, data)
		elif message_type == 'subscribe':
		await self._handle_subscribe(websocket, data)
		elif message_type == 'unsubscribe':
		await self._handle_unsubscribe(websocket, data)
		elif message_type == 'request_data':
		await self._handle_request_data(websocket, data)
		elif message_type == 'control':
		await self._handle_control(websocket, data)
		else:
		logger.warning(f"Unknown message type: {message_type}")
		await self._send_error(websocket, f"Unknown message type: {message_type}")

		except json.JSONDecodeError:
		await self._send_error(websocket, "Invalid JSON message")
		except Exception as e:
		logger.error(f"Error handling message: {e}")
		await self._send_error(websocket, f"Internal error: {str(e)}")

	async def _handle_ping(self, websocket: WebSocket, data: Dict):
		"""Handle ping messages"""
		response = {
		'type': 'pong',
		'timestamp': datetime.now().isoformat(),
		'data': data.get('data', {})
		}
		await self.send_personal_message(response, websocket)

	async def _handle_subscribe(self, websocket: WebSocket, data: Dict):
		"""Handle subscription requests"""
		topics = data.get('topics', [])

		if websocket in self.connection_info:
		self.connection_info[websocket]['subscribed_topics'] = topics

		response = {
		'type': 'subscription_confirmed',
		'timestamp': datetime.now().isoformat(),
		'topics': topics
		}
		await self.send_personal_message(response, websocket)

		logger.info(f"Client subscribed to topics: {topics}")

	async def _handle_unsubscribe(self, websocket: WebSocket, data: Dict):
		"""Handle unsubscription requests"""
		topics = data.get('topics', [])

		if websocket in self.connection_info:
		current_topics = self.connection_info[websocket].get('subscribed_topics', [])
		# Remove unsubscribed topics
		for topic in topics:
		if topic in current_topics:
		current_topics.remove(topic)
		self.connection_info[websocket]['subscribed_topics'] = current_topics

		response = {
		'type': 'unsubscription_confirmed',
		'timestamp': datetime.now().isoformat(),
		'topics': topics
		}
		await self.send_personal_message(response, websocket)

		logger.info(f"Client unsubscribed from topics: {topics}")

	async def _handle_request_data(self, websocket: WebSocket, data: Dict):
		"""Handle data requests"""
		request_type = data.get('request_type', 'unknown')

		response = {
		'type': 'data_response',
		'timestamp': datetime.now().isoformat(),
		'request_type': request_type,
		'data': {}
		}

		# This would typically fetch the requested data
		# For now, just acknowledge the request
		response['data'] = {'status': 'request_received', 'type': request_type}

		await self.send_personal_message(response, websocket)

		logger.info(f"Data request handled: {request_type}")

	async def _handle_control(self, websocket: WebSocket, data: Dict):
		"""Handle control messages"""
		control_type = data.get('control_type', 'unknown')
		control_data = data.get('data', {})

		response = {
		'type': 'control_response',
		'timestamp': datetime.now().isoformat(),
		'control_type': control_type,
		'status': 'received'
		}

		# This would typically execute the control command
		# For now, just acknowledge the control message
		response['data'] = {'status': 'control_received', 'type': control_type}

		await self.send_personal_message(response, websocket)

		logger.info(f"Control message handled: {control_type}")

	async def _send_error(self, websocket: WebSocket, error_message: str):
		"""Send an error message to a client"""
		error_response = {
		'type': 'error',
		'timestamp': datetime.now().isoformat(),
		'error': error_message
		}
		await self.send_personal_message(error_response, websocket)

	async def start_broadcast_loop(self):
		"""Start the broadcast loop for queued messages"""
		self.running = True
		logger.info("WebSocket broadcast loop started")

		while self.running:
		try:
		# Wait for messages in the queue
		message = await asyncio.wait_for(self.broadcast_queue.get(), timeout=1.0)

		if message:
		await self.broadcast(message)

		except asyncio.TimeoutError:
		# No messages in queue, continue
		continue
		except Exception as e:
		logger.error(f"Error in broadcast loop: {e}")
		await asyncio.sleep(1)  # Wait before continuing

	def stop_broadcast_loop(self):
		"""Stop the broadcast loop"""
		self.running = False
		logger.info("WebSocket broadcast loop stopped")

	async def queue_message(self, message: Dict):
		"""Queue a message for broadcasting"""
		await self.broadcast_queue.put(message)

	def get_connection_info(self) -> List[Dict]:
		"""Get information about all active connections"""
		connections = []
		for websocket, info in self.connection_info.items():
		connection_data = info.copy()
		connection_data['connected'] = websocket in self.active_connections
		connections.append(connection_data)
		return connections

	def get_statistics(self) -> Dict:
		"""Get WebSocket manager statistics"""
		return {
		'active_connections': len(self.active_connections),
		'total_connections': self.total_connections,
		'messages_sent': self.messages_sent,
		'messages_received': self.messages_received,
		'broadcast_queue_size': self.broadcast_queue.qsize(),
		'running': self.running
		}

	async def cleanup_inactive_connections(self, max_idle_time: int = 300):
		"""Clean up inactive connections (default: 5 minutes)"""
		current_time = datetime.now()
		inactive_websockets = []

		for websocket, info in self.connection_info.items():
		last_activity = datetime.fromisoformat(info['last_activity'])
		idle_time = (current_time - last_activity).total_seconds()

		if idle_time > max_idle_time:
		inactive_websockets.append(websocket)

		for websocket in inactive_websockets:
		logger.info(f"Cleaning up inactive connection: {info.get('id', 'unknown')}")
		await self.disconnect(websocket)

	async def health_check(self):
		"""Perform health check on all connections"""
		dead_websockets = []

		for websocket in self.active_connections:
		try:
		# Send a ping message
		ping_msg = {
		'type': 'ping',
		'timestamp': datetime.now().isoformat()
		}
		await websocket.send_text(json.dumps(ping_msg))

		except Exception as e:
		logger.warning(f"Health check failed for connection: {e}")
		dead_websockets.append(websocket)

		# Remove dead connections
		for websocket in dead_websockets:
		await self.disconnect(websocket)

		if dead_websockets:
		logger.info(f"Health check removed {len(dead_websockets)} dead connections")

	def get_subscribed_clients(self, topic: str) -> List[WebSocket]:
		"""Get all clients subscribed to a specific topic"""
		subscribed_clients = []

		for websocket, info in self.connection_info.items():
		if websocket in self.active_connections:
		subscribed_topics = info.get('subscribed_topics', [])
		if topic in subscribed_topics:
		subscribed_clients.append(websocket)

		return subscribed_clients

	async def broadcast_to_topic(self, topic: str, message: Dict):
		"""Broadcast a message only to clients subscribed to a specific topic"""
		subscribed_clients = self.get_subscribed_clients(topic)

		if not subscribed_clients:
		return

		# Add topic information to message
		message['topic'] = topic
		message['timestamp'] = datetime.now().isoformat()

		message_json = json.dumps(message)
		disconnected_websockets = []

		for websocket in subscribed_clients:
		try:
		await websocket.send_text(message_json)

		# Update connection info
		if websocket in self.connection_info:
		self.connection_info[websocket]['message_count'] += 1
		self.connection_info[websocket]['last_activity'] = datetime.now()

		self.messages_sent += 1

		except Exception as e:
		logger.error(f"Error broadcasting to topic {topic}: {e}")
		disconnected_websockets.append(websocket)

		# Remove disconnected websockets
		for websocket in disconnected_websockets:
		await self.disconnect(websocket)
