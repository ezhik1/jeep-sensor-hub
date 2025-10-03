require('dotenv').config();

module.exports = {
	// Server configuration
	server: {
		port: process.env.PORT || 3000,
		host: process.env.HOST || '0.0.0.0',
		env: process.env.NODE_ENV || 'development'
	},

	// Database configuration
	database: {
		url: process.env.DATABASE_URL || 'sqlite:./data/jeep-sensor-hub.db',
		logging: process.env.NODE_ENV === 'development'
	},

	// JWT configuration
	jwt: {
		secret: process.env.JWT_SECRET || 'your-secret-key-change-in-production',
		expiresIn: process.env.JWT_EXPIRES_IN || '24h',
		refreshExpiresIn: process.env.JWT_REFRESH_EXPIRES_IN || '7d'
	},

	// Rate limiting configuration
	rateLimit: {
		windowMs: 15 * 60 * 1000, // 15 minutes
		max: 100, // limit each IP to 100 requests per windowMs
		message: 'Too many requests from this IP, please try again later.',
		standardHeaders: true,
		legacyHeaders: false
	},

	// CORS configuration
	cors: {
		origin: process.env.CORS_ORIGIN || '*',
		methods: ['GET', 'POST', 'PUT', 'DELETE', 'OPTIONS'],
		allowedHeaders: ['Content-Type', 'Authorization', 'X-Requested-With'],
		credentials: true
	},

	// Logging configuration
	logging: {
		level: process.env.LOG_LEVEL || 'info',
		file: process.env.LOG_FILE || './logs/app.log',
		maxSize: process.env.LOG_MAX_SIZE || '20m',
		maxFiles: process.env.LOG_MAX_FILES || '14d'
	},

	// WebSocket configuration
	websocket: {
		heartbeatInterval: 30000, // 30 seconds
		clientTimeout: 120000, // 2 minutes
		maxClients: 100
	},

	// Sensor hub configuration
	sensorHub: {
		url: process.env.SENSOR_HUB_URL || 'http://localhost:8000',
		timeout: 5000,
		retryAttempts: 3,
		retryDelay: 1000
	},

	// Security configuration
	security: {
		bcryptRounds: 12,
		sessionTimeout: 24 * 60 * 60 * 1000, // 24 hours
		maxLoginAttempts: 5,
		lockoutDuration: 15 * 60 * 1000 // 15 minutes
	},

	// File upload configuration
	uploads: {
		maxFileSize: 10 * 1024 * 1024, // 10MB
		allowedTypes: ['image/jpeg', 'image/png', 'text/csv', 'application/json'],
		uploadDir: './uploads'
	},

	// Monitoring configuration
	monitoring: {
		healthCheckInterval: 30000, // 30 seconds
		metricsCollection: true,
		performanceMonitoring: true
	}
};
