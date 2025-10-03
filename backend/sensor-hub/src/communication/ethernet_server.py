"""
Ethernet TCP Server for Sensor Hub
Handles high-speed communication with ESP32 display modules
"""

import asyncio
import json
import logging
import time
from typing import Dict, List, Optional, Callable, Any
from dataclasses import dataclass
from datetime import datetime

logger = logging.getLogger(__name__)

@dataclass
class DisplayClient:
	"""Represents a connected ESP32 display client"""
	client_id: str
	reader: asyncio.StreamReader
	writer: asyncio.StreamWriter
	module_id: str
	capabilities: List[str]
	connected_at: datetime
	last_heartbeat: datetime
	ip_address: str
	port: int
	status: str = 'connected'  # connected, disconnected, error

class EthernetServer:
	def __init__(self, host: str = '0.0.0.0', port: int = 8080,
				 data_callback: Optional[Callable] = None):
		"""
		Initialize Ethernet TCP server

		Args:
			host: Host address to bind to
			port: Port to listen on
			data_callback: Callback function for received data
		"""
		self.host = host
		self.port = port
		self.data_callback = data_callback

		# Server state
		self.server = None
		self.running = False
		self.clients: Dict[str, DisplayClient] = {}

		# Configuration
		self.max_clients = 10
		self.heartbeat_timeout = 30  # seconds
		self.buffer_size = 4096
		self.cleanup_interval = 60  # seconds

		# Statistics
		self.stats = {
			'connections_total': 0,
			'connections_active': 0,
			'messages_received': 0,
			'messages_sent': 0,
			'bytes_received': 0,
			'bytes_sent': 0,
			'errors': 0,
			'start_time': None
		}

		# Message handlers
		self.message_handlers = {
			'sensor_data': self._handle_sensor_data,
			'engine_state': self._handle_engine_state,
			'power_state': self._handle_power_state,
			'heartbeat': self._handle_heartbeat,
			'command': self._handle_command,
			'response': self._handle_response,
			'status': self._handle_status,
			'alert': self._handle_alert
		}

		logger.info(f"Ethernet server initialized for {host}:{port}")

	async def start(self):
		"""Start the Ethernet server"""
		try:
			self.server = await asyncio.start_server(
				self._handle_client,
				self.host,
				self.port,
				limit=self.buffer_size
			)

			self.running = True
			self.stats['start_time'] = datetime.now()

			logger.info(f"Ethernet server started on {self.host}:{self.port}")

			# Start background tasks
			asyncio.create_task(self._heartbeat_monitor())
			asyncio.create_task(self._cleanup_task())

		except Exception as e:
			logger.error(f"Failed to start Ethernet server: {e}")
			raise

	async def stop(self):
		"""Stop the Ethernet server"""
		try:
			self.running = False

			# Close all client connections
			for client in list(self.clients.values()):
				await self._disconnect_client(client)

			# Close server
			if self.server:
				self.server.close()
				await self.server.wait_closed()

			logger.info("Ethernet server stopped")

		except Exception as e:
			logger.error(f"Error stopping Ethernet server: {e}")

	async def _handle_client(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
		"""Handle new client connection"""
		client_id = None
		try:
			# Get client address
			peername = writer.get_extra_info('peername')
			ip_address = peername[0] if peername else 'unknown'
			port = peername[1] if peername else 0

			# Check if we can accept more clients
			if len(self.clients) >= self.max_clients:
				logger.warning(f"Rejecting connection from {ip_address}:{port} - max clients reached")
				writer.close()
				await writer.wait_closed()
				return

			# Generate client ID
			client_id = f"{ip_address}_{port}_{int(time.time())}"

			# Create client object
			client = DisplayClient(
				client_id=client_id,
				reader=reader,
				writer=writer,
				module_id='unknown',
				capabilities=[],
				connected_at=datetime.now(),
				last_heartbeat=datetime.now(),
				ip_address=ip_address,
				port=port
			)

			# Add to clients list
			self.clients[client_id] = client
			self.stats['connections_total'] += 1
			self.stats['connections_active'] += 1

			logger.info(f"Client connected: {client_id} from {ip_address}:{port}")

			# Send welcome message
			await self._send_message(client, {
				'type': 'welcome',
				'client_id': client_id,
				'timestamp': datetime.now().isoformat(),
				'server_info': {
					'name': 'Jeep Sensor Hub',
					'version': '1.0.0',
					'capabilities': ['sensor_data', 'engine_state', 'power_state', 'commands']
				}
			})

			# Handle client communication
			await self._client_communication_loop(client)

		except Exception as e:
			logger.error(f"Error handling client {client_id}: {e}")
			self.stats['errors'] += 1
		finally:
			# Clean up client
			if client_id and client_id in self.clients:
				await self._disconnect_client(self.clients[client_id])

	async def _client_communication_loop(self, client: DisplayClient):
		"""Main communication loop for a client"""
		try:
			while self.running and client.status == 'connected':
				# Read message from client
				message = await self._read_message(client)
				if not message:
					break

				# Process message
				await self._process_message(client, message)

		except asyncio.CancelledError:
			logger.debug(f"Client communication cancelled for {client.client_id}")
		except Exception as e:
			logger.error(f"Error in client communication loop for {client.client_id}: {e}")
			client.status = 'error'

	async def _read_message(self, client: DisplayClient) -> Optional[Dict]:
		"""Read a message from client"""
		try:
			# Read message length (4 bytes)
			length_bytes = await client.reader.readexactly(4)
			message_length = int.from_bytes(length_bytes, 'big')

			# Read message data
			message_data = await client.reader.readexactly(message_length)
			message = json.loads(message_data.decode('utf-8'))

			# Update statistics
			self.stats['messages_received'] += 1
			self.stats['bytes_received'] += message_length + 4

			# Update heartbeat
			client.last_heartbeat = datetime.now()

			return message

		except asyncio.IncompleteReadError:
			# Client disconnected
			return None
		except json.JSONDecodeError as e:
			logger.error(f"Invalid JSON from client {client.client_id}: {e}")
			return None
		except Exception as e:
			logger.error(f"Error reading message from client {client.client_id}: {e}")
			return None

	async def _process_message(self, client: DisplayClient, message: Dict):
		"""Process a received message"""
		try:
			message_type = message.get('type')
			if not message_type:
				logger.warning(f"Message from {client.client_id} missing type field")
				return

			# Update client info if provided
			if 'module_id' in message:
				client.module_id = message['module_id']
			if 'capabilities' in message:
				client.capabilities = message['capabilities']

			# Route to appropriate handler
			handler = self.message_handlers.get(message_type)
			if handler:
				await handler(client, message)
			else:
				logger.warning(f"Unknown message type from {client.client_id}: {message_type}")

		except Exception as e:
			logger.error(f"Error processing message from {client.client_id}: {e}")

	async def _send_message(self, client: DisplayClient, message: Dict):
		"""Send a message to a client"""
		try:
			# Convert message to JSON
			message_json = json.dumps(message, default=str)
			message_bytes = message_json.encode('utf-8')
			message_length = len(message_bytes)

			# Send length and message
			client.writer.write(message_length.to_bytes(4, 'big'))
			client.writer.write(message_bytes)
			await client.writer.drain()

			# Update statistics
			self.stats['messages_sent'] += 1
			self.stats['bytes_sent'] += message_length + 4

		except Exception as e:
			logger.error(f"Error sending message to {client.client_id}: {e}")
			client.status = 'error'

	async def broadcast_message(self, message: Dict, exclude_client: str = None):
		"""Broadcast message to all connected clients"""
		disconnected_clients = []

		for client_id, client in self.clients.items():
			if client.status == 'connected' and client_id != exclude_client:
				try:
					await self._send_message(client, message)
				except Exception as e:
					logger.error(f"Error broadcasting to {client_id}: {e}")
					disconnected_clients.append(client_id)

		# Clean up disconnected clients
		for client_id in disconnected_clients:
			if client_id in self.clients:
				await self._disconnect_client(self.clients[client_id])

	async def _disconnect_client(self, client: DisplayClient):
		"""Disconnect a client"""
		try:
			client.status = 'disconnected'
			client.writer.close()
			await client.writer.wait_closed()

			# Remove from clients list
			if client.client_id in self.clients:
				del self.clients[client.client_id]
				self.stats['connections_active'] -= 1

			logger.info(f"Client disconnected: {client.client_id}")

		except Exception as e:
			logger.error(f"Error disconnecting client {client.client_id}: {e}")

	# Message handlers
	async def _handle_sensor_data(self, client: DisplayClient, message: Dict):
		"""Handle sensor data from client"""
		try:
			logger.debug(f"Received sensor data from {client.client_id}: {message.get('data_type', 'unknown')}")

			# Forward to callback if provided
			if self.data_callback:
				await self.data_callback('sensor_data', message)

		except Exception as e:
			logger.error(f"Error handling sensor data from {client.client_id}: {e}")

	async def _handle_engine_state(self, client: DisplayClient, message: Dict):
		"""Handle engine state from client"""
		try:
			logger.debug(f"Received engine state from {client.client_id}: {message.get('state', 'unknown')}")

			# Forward to callback if provided
			if self.data_callback:
				await self.data_callback('engine_state', message)

		except Exception as e:
			logger.error(f"Error handling engine state from {client.client_id}: {e}")

	async def _handle_power_state(self, client: DisplayClient, message: Dict):
		"""Handle power state from client"""
		try:
			logger.debug(f"Received power state from {client.client_id}: {message.get('state', 'unknown')}")

			# Forward to callback if provided
			if self.data_callback:
				await self.data_callback('power_state', message)

		except Exception as e:
			logger.error(f"Error handling power state from {client.client_id}: {e}")

	async def _handle_heartbeat(self, client: DisplayClient, message: Dict):
		"""Handle heartbeat from client"""
		try:
			client.last_heartbeat = datetime.now()
			logger.debug(f"Received heartbeat from {client.client_id}")

		except Exception as e:
			logger.error(f"Error handling heartbeat from {client.client_id}: {e}")

	async def _handle_command(self, client: DisplayClient, message: Dict):
		"""Handle command from client"""
		try:
			command = message.get('command', 'unknown')
			logger.info(f"Received command from {client.client_id}: {command}")

			# Forward to callback if provided
			if self.data_callback:
				await self.data_callback('command', message)

		except Exception as e:
			logger.error(f"Error handling command from {client.client_id}: {e}")

	async def _handle_response(self, client: DisplayClient, message: Dict):
		"""Handle response from client"""
		try:
			logger.debug(f"Received response from {client.client_id}: {message.get('response_type', 'unknown')}")

			# Forward to callback if provided
			if self.data_callback:
				await self.data_callback('response', message)

		except Exception as e:
			logger.error(f"Error handling response from {client.client_id}: {e}")

	async def _handle_status(self, client: DisplayClient, message: Dict):
		"""Handle status update from client"""
		try:
			status = message.get('status', 'unknown')
			logger.debug(f"Received status from {client.client_id}: {status}")

			# Forward to callback if provided
			if self.data_callback:
				await self.data_callback('status', message)

		except Exception as e:
			logger.error(f"Error handling status from {client.client_id}: {e}")

	async def _handle_alert(self, client: DisplayClient, message: Dict):
		"""Handle alert from client"""
		try:
			alert_type = message.get('alert_type', 'unknown')
			alert_message = message.get('message', 'No message')
			logger.warning(f"Received alert from {client.client_id}: {alert_type} - {alert_message}")

			# Forward to callback if provided
			if self.data_callback:
				await self.data_callback('alert', message)

		except Exception as e:
			logger.error(f"Error handling alert from {client.client_id}: {e}")

	# Background tasks
	async def _heartbeat_monitor(self):
		"""Monitor client heartbeats and disconnect inactive clients"""
		while self.running:
			try:
				current_time = datetime.now()
				disconnected_clients = []

				for client_id, client in self.clients.items():
					if client.status == 'connected':
						time_since_heartbeat = (current_time - client.last_heartbeat).total_seconds()
						if time_since_heartbeat > self.heartbeat_timeout:
							logger.warning(f"Client {client_id} heartbeat timeout, disconnecting")
							disconnected_clients.append(client_id)

				# Disconnect timed out clients
				for client_id in disconnected_clients:
					if client_id in self.clients:
						await self._disconnect_client(self.clients[client_id])

				await asyncio.sleep(10)  # Check every 10 seconds

			except Exception as e:
				logger.error(f"Error in heartbeat monitor: {e}")
				await asyncio.sleep(10)

	async def _cleanup_task(self):
		"""Periodic cleanup task"""
		while self.running:
			try:
				# Remove disconnected clients
				disconnected_clients = [
					client_id for client_id, client in self.clients.items()
					if client.status == 'disconnected'
				]

				for client_id in disconnected_clients:
					if client_id in self.clients:
						del self.clients[client_id]

				await asyncio.sleep(self.cleanup_interval)

			except Exception as e:
				logger.error(f"Error in cleanup task: {e}")
				await asyncio.sleep(self.cleanup_interval)

	# Public API methods
	def get_client_count(self) -> int:
		"""Get number of connected clients"""
		return len([c for c in self.clients.values() if c.status == 'connected'])

	def get_client_info(self, client_id: str) -> Optional[Dict]:
		"""Get information about a specific client"""
		if client_id not in self.clients:
			return None

		client = self.clients[client_id]
		return {
			'client_id': client.client_id,
			'module_id': client.module_id,
			'capabilities': client.capabilities,
			'connected_at': client.connected_at.isoformat(),
			'last_heartbeat': client.last_heartbeat.isoformat(),
			'ip_address': client.ip_address,
			'port': client.port,
			'status': client.status
		}

	def get_all_clients(self) -> List[Dict]:
		"""Get information about all clients"""
		return [self.get_client_info(client_id) for client_id in self.clients.keys()]

	def get_statistics(self) -> Dict:
		"""Get server statistics"""
		uptime = None
		if self.stats['start_time']:
			uptime = (datetime.now() - self.stats['start_time']).total_seconds()

		return {
			**self.stats,
			'uptime_seconds': uptime,
			'clients_connected': self.get_client_count(),
			'clients_total': len(self.clients)
		}

	async def send_to_client(self, client_id: str, message: Dict) -> bool:
		"""Send message to specific client"""
		if client_id not in self.clients:
			logger.warning(f"Client {client_id} not found")
			return False

		client = self.clients[client_id]
		if client.status != 'connected':
			logger.warning(f"Client {client_id} not connected")
			return False

		try:
			await self._send_message(client, message)
			return True
		except Exception as e:
			logger.error(f"Error sending to client {client_id}: {e}")
			return False
