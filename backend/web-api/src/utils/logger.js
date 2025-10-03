/**
 * Winston Logger Configuration
 * Structured logging for the Jeep Sensor Hub Backend API
 */

const winston = require('winston');
const path = require('path');

// Define log levels
const logLevels = {
	error: 0,
	warn: 1,
	info: 2,
	http: 3,
	debug: 4,
};

// Define log colors
const logColors = {
	error: 'red',
	warn: 'yellow',
	info: 'green',
	http: 'magenta',
	debug: 'white',
};

// Add colors to Winston
winston.addColors(logColors);

// Define log format
const logFormat = winston.format.combine(
	winston.format.timestamp({ format: 'YYYY-MM-DD HH:mm:ss:ms' }),
	winston.format.colorize({ all: true }),
	winston.format.printf(
		(info) => `${info.timestamp} ${info.level}: ${info.message}`,
	),
);

// Define file format (without colors)
const fileFormat = winston.format.combine(
	winston.format.timestamp({ format: 'YYYY-MM-DD HH:mm:ss:ms' }),
	winston.format.errors({ stack: true }),
	winston.format.json(),
);

// Create logs directory if it doesn't exist
const fs = require('fs');
const logsDir = path.join(__dirname, '../../logs');
if (!fs.existsSync(logsDir)) {
	fs.mkdirSync(logsDir, { recursive: true });
}

// Create Winston logger
const logger = winston.createLogger({
	levels: logLevels,
	level: process.env.LOG_LEVEL || 'info',
	format: fileFormat,
	transports: [
		// Error log file
		new winston.transports.File({
			filename: path.join(logsDir, 'error.log'),
			level: 'error',
			maxsize: 5242880, // 5MB
			maxFiles: 5,
		}),

		// Combined log file
		new winston.transports.File({
			filename: path.join(logsDir, 'combined.log'),
			maxsize: 5242880, // 5MB
			maxFiles: 5,
		}),

		// HTTP requests log file
		new winston.transports.File({
			filename: path.join(logsDir, 'http.log'),
			level: 'http',
			maxsize: 5242880, // 5MB
			maxFiles: 5,
		}),
	],
});

// Add console transport for development
if (process.env.NODE_ENV !== 'production') {
	logger.add(new winston.transports.Console({
		format: logFormat,
	}));
}

// Create a stream object for Morgan HTTP logging
logger.stream = {
	write: (message) => {
		logger.http(message.trim());
	},
};

// Helper methods for structured logging
logger.logSensorData = (sensorType, data, source = 'unknown') => {
	logger.info('Sensor data received', {
		sensorType,
		source,
		timestamp: new Date().toISOString(),
		data: JSON.stringify(data),
	});
};

logger.logGPSData = (coordinates, speed, altitude) => {
	logger.info('GPS data received', {
		coordinates,
		speed,
		altitude,
		timestamp: new Date().toISOString(),
	});
};

logger.logPowerEvent = (eventType, voltage, current, source) => {
	logger.info('Power event occurred', {
		eventType,
		voltage,
		current,
		source,
		timestamp: new Date().toISOString(),
	});
};

logger.logCoolingEvent = (eventType, temperature, fanSpeed, action) => {
	logger.info('Cooling event occurred', {
		eventType,
		temperature,
		fanSpeed,
		action,
		timestamp: new Date().toISOString(),
	});
};

logger.logWebSocketEvent = (eventType, connectionId, data) => {
	logger.info('WebSocket event', {
		eventType,
		connectionId,
		dataSize: data ? JSON.stringify(data).length : 0,
		timestamp: new Date().toISOString(),
	});
};

logger.logAPICall = (method, endpoint, statusCode, responseTime, userId = null) => {
	logger.info('API call completed', {
		method,
		endpoint,
		statusCode,
		responseTime: `${responseTime}ms`,
		userId,
		timestamp: new Date().toISOString(),
	});
};

logger.logDatabaseOperation = (operation, table, duration, success) => {
	logger.info('Database operation', {
		operation,
		table,
		duration: `${duration}ms`,
		success,
		timestamp: new Date().toISOString(),
	});
};

logger.logSecurityEvent = (eventType, ip, userAgent, details) => {
	logger.warn('Security event detected', {
		eventType,
		ip,
		userAgent,
		details,
		timestamp: new Date().toISOString(),
	});
};

logger.logPerformanceMetric = (metricName, value, unit, context = {}) => {
	logger.info('Performance metric', {
		metricName,
		value,
		unit,
		context,
		timestamp: new Date().toISOString(),
	});
};

// Export logger instance
module.exports = logger;
