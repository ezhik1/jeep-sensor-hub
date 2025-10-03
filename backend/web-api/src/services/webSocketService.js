const WebSocket = require('ws');
const logger = require('../utils/logger');

class WebSocketService {
	constructor(server) {
		this.wss = new WebSocket.Server({ server });
		this.clients = new Set();
		this.heartbeatInterval = null;
		this.broadcastQueue = [];
		this.isInitialized = false;

		this._setupWebSocket();
	}

	_setupWebSocket() {
		try {
			this.wss.on('connection', (ws, req) => {
				this._handleConnection(ws, req);
			});

			this.wss.on('error', (error) => {
				logger.error('WebSocket server error:', error);
			});

			// Start heartbeat
			this._startHeartbeat();

			this.isInitialized = true;
			logger.logWebSocketEvent('WebSocket service initialized');
		} catch (error) {
			logger.error('Error setting up WebSocket service:', error);
		}
	}

	_handleConnection(ws, req) {
		try {
			const clientId = this._generateClientId();
			const clientInfo = {
				id: clientId,
				ws,
				ip: req.socket.remoteAddress,
				userAgent: req.headers['user-agent'],
				connectedAt: new Date(),
				lastHeartbeat: new Date(),
				subscriptions: new Set()
			};

			// Add client to set
			this.clients.add(clientInfo);

			logger.logWebSocketEvent('Client connected', {
				clientId,
				ip: clientInfo.ip,
				totalClients: this.clients.size
			});

			// Send welcome message
			this._sendToClient(clientInfo, {
				type: 'connection',
				clientId,
				timestamp: new Date(),
				message: 'Connected to Jeep Sensor Hub WebSocket'
			});

			// Handle client messages
			ws.on('message', (data) => {
				this._handleClientMessage(clientInfo, data);
			});

			// Handle client disconnect
			ws.on('close', () => {
				this._handleClientDisconnect(clientInfo);
			});

			// Handle client errors
			ws.on('error', (error) => {
				logger.error('WebSocket client error:', error);
				this._handleClientDisconnect(clientInfo);
			});

			// Handle pong responses
			ws.on('pong', () => {
				clientInfo.lastHeartbeat = new Date();
			});

		} catch (error) {
			logger.error('Error handling WebSocket connection:', error);
		}
	}

	_handleClientMessage(clientInfo, data) {
		try {
			let message;

			try {
				message = JSON.parse(data.toString());
			} catch (parseError) {
				logger.error('Error parsing WebSocket message:', parseError);
				return;
			}

			const { type, payload } = message;

			switch (type) {
				case 'subscribe':
					this._handleSubscription(clientInfo, payload);
					break;
				case 'unsubscribe':
					this._handleUnsubscription(clientInfo, payload);
					break;
				case 'ping':
					this._sendToClient(clientInfo, { type: 'pong', timestamp: new Date() });
					break;
				case 'request_data':
					this._handleDataRequest(clientInfo, payload);
					break;
				default:
					logger.warn('Unknown WebSocket message type:', type);
			}

			logger.logWebSocketEvent('Client message received', {
				clientId: clientInfo.id,
				messageType: type
			});

		} catch (error) {
			logger.error('Error handling client message:', error);
		}
	}

	_handleSubscription(clientInfo, payload) {
		try {
			const { channels = [] } = payload;

			if (!Array.isArray(channels)) {
				this._sendToClient(clientInfo, {
					type: 'error',
					message: 'Channels must be an array',
					timestamp: new Date()
				});
				return;
			}

			// Add subscriptions
			channels.forEach(channel => {
				clientInfo.subscriptions.add(channel);
			});

			this._sendToClient(clientInfo, {
				type: 'subscription_confirmed',
				channels: Array.from(clientInfo.subscriptions),
				timestamp: new Date()
			});

			logger.logWebSocketEvent('Client subscribed', {
				clientId: clientInfo.id,
				channels: Array.from(clientInfo.subscriptions)
			});

		} catch (error) {
			logger.error('Error handling subscription:', error);
		}
	}

	_handleUnsubscription(clientInfo, payload) {
		try {
			const { channels = [] } = payload;

			if (!Array.isArray(channels)) {
				this._sendToClient(clientInfo, {
					type: 'error',
					message: 'Channels must be an array',
					timestamp: new Date()
				});
				return;
			}

			// Remove subscriptions
			channels.forEach(channel => {
				clientInfo.subscriptions.delete(channel);
			});

			this._sendToClient(clientInfo, {
				type: 'unsubscription_confirmed',
				channels: Array.from(clientInfo.subscriptions),
				timestamp: new Date()
			});

			logger.logWebSocketEvent('Client unsubscribed', {
				clientId: clientInfo.id,
				channels: Array.from(clientInfo.subscriptions)
			});

		} catch (error) {
			logger.error('Error handling unsubscription:', error);
		}
	}

	_handleDataRequest(clientInfo, payload) {
		try {
			const { dataType, filters = {} } = payload;

			// This would typically fetch data from the appropriate service
			// For now, just acknowledge the request
			this._sendToClient(clientInfo, {
				type: 'data_request_acknowledged',
				dataType,
				filters,
				timestamp: new Date(),
				message: 'Data request received'
			});

		} catch (error) {
			logger.error('Error handling data request:', error);
		}
	}

	_handleClientDisconnect(clientInfo) {
		try {
			// Remove client from set
			this.clients.delete(clientInfo);

			logger.logWebSocketEvent('Client disconnected', {
				clientId: clientInfo.id,
				totalClients: this.clients.size
			});

		} catch (error) {
			logger.error('Error handling client disconnect:', error);
		}
	}

	_sendToClient(clientInfo, message) {
		try {
			if (clientInfo.ws.readyState === WebSocket.OPEN) {
				clientInfo.ws.send(JSON.stringify(message));
			}
		} catch (error) {
			logger.error('Error sending message to client:', error);
		}
	}

	broadcast(data, channels = ['all']) {
		try {
			const message = {
				type: 'data_update',
				data,
				channels,
				timestamp: new Date()
			};

			// Add to broadcast queue for processing
			this.broadcastQueue.push({ message, channels });

			// Process queue if not already processing
			if (this.broadcastQueue.length === 1) {
				this._processBroadcastQueue();
			}

		} catch (error) {
			logger.error('Error broadcasting data:', error);
		}
	}

	async _processBroadcastQueue() {
		try {
			while (this.broadcastQueue.length > 0) {
				const { message, channels } = this.broadcastQueue.shift();

				// Send to subscribed clients
				for (const client of this.clients) {
					if (client.ws.readyState === WebSocket.OPEN) {
						// Check if client is subscribed to any of the channels
						const hasMatchingSubscription = channels.some(channel =>
							channel === 'all' || client.subscriptions.has(channel)
						);

						if (hasMatchingSubscription) {
							this._sendToClient(client, message);
						}
					}
				}
			}
		} catch (error) {
			logger.error('Error processing broadcast queue:', error);
		}
	}

	broadcastToChannel(channel, data) {
		try {
			this.broadcast(data, [channel]);
		} catch (error) {
			logger.error('Error broadcasting to channel:', error);
		}
	}

	broadcastSensorData(sensorData) {
		try {
			this.broadcastToChannel('sensors', {
				type: 'sensor_update',
				sensorData
			});
		} catch (error) {
			logger.error('Error broadcasting sensor data:', error);
		}
	}

	broadcastGPSData(gpsData) {
		try {
			this.broadcastToChannel('gps', {
				type: 'gps_update',
				gpsData
			});
		} catch (error) {
			logger.error('Error broadcasting GPS data:', error);
		}
	}

	broadcastPowerData(powerData) {
		try {
			this.broadcastToChannel('power', {
				type: 'power_update',
				powerData
			});
		} catch (error) {
			logger.error('Error broadcasting power data:', error);
		}
	}

	broadcastCoolingData(coolingData) {
		try {
			this.broadcastToChannel('cooling', {
				type: 'cooling_update',
				coolingData
			});
		} catch (error) {
			logger.error('Error broadcasting cooling data:', error);
		}
	}

	broadcastSystemAlert(alert) {
		try {
			this.broadcastToChannel('alerts', {
				type: 'system_alert',
				alert
			});
		} catch (error) {
			logger.error('Error broadcasting system alert:', error);
		}
	}

	_startHeartbeat() {
		try {
			this.heartbeatInterval = setInterval(() => {
				this._sendHeartbeat();
			}, 30000); // 30 seconds

			logger.logWebSocketEvent('WebSocket heartbeat started');
		} catch (error) {
			logger.error('Error starting WebSocket heartbeat:', error);
		}
	}

	_sendHeartbeat() {
		try {
			const now = new Date();
			const heartbeatMessage = {
				type: 'heartbeat',
				timestamp: now,
				serverTime: now.toISOString()
			};

			// Send heartbeat to all connected clients
			for (const client of this.clients) {
				if (client.ws.readyState === WebSocket.OPEN) {
					// Send ping
					client.ws.ping();

					// Send heartbeat message
					this._sendToClient(client, heartbeatMessage);
				}
			}

			// Clean up stale connections (no heartbeat for 2 minutes)
			const staleThreshold = new Date(now.getTime() - 120000);
			for (const client of this.clients) {
				if (client.lastHeartbeat < staleThreshold) {
					logger.logWebSocketEvent('Stale client connection detected', {
						clientId: client.id
					});
					client.ws.terminate();
				}
			}

		} catch (error) {
			logger.error('Error sending WebSocket heartbeat:', error);
		}
	}

	getClientStats() {
		try {
			const stats = {
				totalClients: this.clients.size,
				activeConnections: 0,
				subscriptionCounts: {},
				uptime: this.isInitialized ? Date.now() - this._startTime : 0
			};

			for (const client of this.clients) {
				if (client.ws.readyState === WebSocket.OPEN) {
					stats.activeConnections++;
				}

				// Count subscriptions
				for (const channel of client.subscriptions) {
					stats.subscriptionCounts[channel] = (stats.subscriptionCounts[channel] || 0) + 1;
				}
			}

			return stats;
		} catch (error) {
			logger.error('Error getting WebSocket client stats:', error);
			return {};
		}
	}

	_generateClientId() {
		return 'client_' + Date.now().toString(36) + Math.random().toString(36).substr(2);
	}

	cleanup() {
		try {
			// Stop heartbeat
			if (this.heartbeatInterval) {
				clearInterval(this.heartbeatInterval);
				this.heartbeatInterval = null;
			}

			// Close all client connections
			for (const client of this.clients) {
				if (client.ws.readyState === WebSocket.OPEN) {
					client.ws.close(1000, 'Server shutdown');
				}
			}

			// Close WebSocket server
			if (this.wss) {
				this.wss.close();
			}

			logger.logWebSocketEvent('WebSocket service cleanup completed');
		} catch (error) {
			logger.error('Error during WebSocket service cleanup:', error);
		}
	}
}

module.exports = WebSocketService;
