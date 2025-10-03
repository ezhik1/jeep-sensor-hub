#!/usr/bin/env node
/**
 * Jeep Sensor Hub Backend API Server
 * Node.js/Express middleware for sensor data processing and web frontend
 */

const express = require('express');
const cors = require('cors');
const helmet = require('helmet');
const morgan = require('morgan');
const compression = require('compression');
const path = require('path');
require('dotenv').config();

// Import middleware and routes
const { errorHandler } = require('./middleware/errorHandler');
const { requestLogger } = require('./middleware/requestLogger');
const { rateLimiter } = require('./middleware/rateLimiter');
const { authMiddleware } = require('./middleware/auth');

// Import route handlers
const sensorRoutes = require('./routes/sensors');
const gpsRoutes = require('./routes/gps');
const powerRoutes = require('./routes/power');
const coolingRoutes = require('./routes/cooling');
const systemRoutes = require('./routes/system');
const dataRoutes = require('./routes/data');

// Import services
const { SensorDataService } = require('./services/sensorDataService');
const { WebSocketService } = require('./services/webSocketService');
const { DatabaseService } = require('./services/databaseService');

// Initialize Express app
const app = express();
const PORT = process.env.PORT || 3000;

// Security middleware
app.use(helmet({
	contentSecurityPolicy: {
		directives: {
			defaultSrc: ["'self'"],
			styleSrc: ["'self'", "'unsafe-inline'"],
			scriptSrc: ["'self'"],
			imgSrc: ["'self'", "data:", "https:"],
		},
	},
}));

// CORS configuration
app.use(cors({
	origin: process.env.FRONTEND_URL || 'http://localhost:5173',
	credentials: true,
	methods: ['GET', 'POST', 'PUT', 'DELETE', 'OPTIONS'],
	allowedHeaders: ['Content-Type', 'Authorization', 'X-Requested-With'],
}));

// Request parsing middleware
app.use(express.json({ limit: '10mb' }));
app.use(express.urlencoded({ extended: true, limit: '10mb' }));

// Compression middleware
app.use(compression());

// Logging middleware
app.use(morgan('combined'));
app.use(requestLogger);

// Rate limiting
app.use(rateLimiter);

// Static file serving (for production builds)
if (process.env.NODE_ENV === 'production') {
	app.use(express.static(path.join(__dirname, '../../web-frontend/dist')));
}

// Health check endpoint
app.get('/health', (req, res) => {
	res.status(200).json({
		status: 'healthy',
		timestamp: new Date().toISOString(),
		version: process.env.npm_package_version || '1.0.0',
		environment: process.env.NODE_ENV || 'development',
	});
});

// API routes
app.use('/api/sensors', sensorRoutes);
app.use('/api/gps', gpsRoutes);
app.use('/api/power', powerRoutes);
app.use('/api/cooling', coolingRoutes);
app.use('/api/system', systemRoutes);
app.use('/api/data', dataRoutes);

// WebSocket endpoint for real-time data
const server = require('http').createServer(app);
const WebSocket = require('ws');
const wss = new WebSocket.Server({ server });

// Initialize WebSocket service
const wsService = new WebSocketService(wss);

// WebSocket connection handling
wss.on('connection', (ws, req) => {
	console.log('New WebSocket connection established');

	ws.on('message', (message) => {
		try {
			const data = JSON.parse(message);
			wsService.handleMessage(ws, data);
		} catch (error) {
			console.error('WebSocket message error:', error);
		}
	});

	ws.on('close', () => {
		console.log('WebSocket connection closed');
		wsService.removeConnection(ws);
	});

	ws.on('error', (error) => {
		console.error('WebSocket error:', error);
	});
});

// Error handling middleware (must be last)
app.use(errorHandler);

// 404 handler for unmatched routes
app.use('*', (req, res) => {
	res.status(404).json({
		error: 'Route not found',
		path: req.originalUrl,
		method: req.method,
	});
});

// Start server
server.listen(PORT, () => {
	console.log(`🚗 Jeep Sensor Hub Backend API running on port ${PORT}`);
	console.log(`📊 Environment: ${process.env.NODE_ENV || 'development'}`);
	console.log(`🔌 WebSocket server ready for real-time connections`);
	console.log(`📡 API endpoints available at http://localhost:${PORT}/api`);

	// Initialize services
	initializeServices();
});

// Graceful shutdown handling
process.on('SIGTERM', () => {
	console.log('SIGTERM received, shutting down gracefully...');
	server.close(() => {
		console.log('Server closed');
		process.exit(0);
	});
});

process.on('SIGINT', () => {
	console.log('SIGINT received, shutting down gracefully...');
	server.close(() => {
		console.log('Server closed');
		process.exit(0);
	});
});

// Initialize all services
async function initializeServices() {
	try {
		// Initialize database service
		const dbService = new DatabaseService();
		await dbService.initialize();
		console.log('✅ Database service initialized');

		// Initialize sensor data service
		const sensorService = new SensorDataService(dbService);
		console.log('✅ Sensor data service initialized');

		// Start WebSocket broadcasting
		wsService.startBroadcasting(sensorService);
		console.log('✅ WebSocket broadcasting started');

	} catch (error) {
		console.error('❌ Service initialization failed:', error);
		process.exit(1);
	}
}

module.exports = { app, server };
