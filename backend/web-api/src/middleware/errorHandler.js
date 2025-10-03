/**
 * Global Error Handler Middleware
 * Handles all errors thrown in the application
 */

const logger = require('../utils/logger');

// Error handler middleware
const errorHandler = (err, req, res, next) => {
	// Log the error
	logger.error('Error occurred:', {
		error: err.message,
		stack: err.stack,
		url: req.originalUrl,
		method: req.method,
		ip: req.ip,
		userAgent: req.get('User-Agent'),
		timestamp: new Date().toISOString(),
	});

	// Set default error values
	const statusCode = err.statusCode || 500;
	const message = err.message || 'Internal Server Error';

	// Don't leak error details in production
	const errorResponse = {
		error: {
			message: process.env.NODE_ENV === 'production' ? 'Internal Server Error' : message,
			status: statusCode,
			timestamp: new Date().toISOString(),
			path: req.originalUrl,
		},
	};

	// Add validation errors if they exist
	if (err.name === 'ValidationError') {
		errorResponse.error.details = err.details;
		errorResponse.error.type = 'ValidationError';
	}

	// Add database errors if they exist
	if (err.name === 'SequelizeValidationError' || err.name === 'SequelizeUniqueConstraintError') {
		errorResponse.error.details = err.errors?.map(e => ({
			field: e.path,
			message: e.message,
			value: e.value,
		}));
		errorResponse.error.type = 'DatabaseError';
	}

	// Add JWT errors if they exist
	if (err.name === 'JsonWebTokenError') {
		errorResponse.error.type = 'AuthenticationError';
		errorResponse.error.message = 'Invalid token';
	}

	if (err.name === 'TokenExpiredError') {
		errorResponse.error.type = 'AuthenticationError';
		errorResponse.error.message = 'Token expired';
	}

	// Send error response
	res.status(statusCode).json(errorResponse);
};

// Async error wrapper for route handlers
const asyncHandler = (fn) => {
	return (req, res, next) => {
		Promise.resolve(fn(req, res, next)).catch(next);
	};
};

// Custom error class for application errors
class AppError extends Error {
	constructor(message, statusCode = 500, details = null) {
		super(message);
		this.statusCode = statusCode;
		this.details = details;
		this.name = this.constructor.name;
		Error.captureStackTrace(this, this.constructor);
	}
}

// Specific error classes
class ValidationError extends AppError {
	constructor(message, details = null) {
		super(message, 400, details);
	}
}

class AuthenticationError extends AppError {
	constructor(message = 'Authentication required') {
		super(message, 401);
	}
}

class AuthorizationError extends AppError {
	constructor(message = 'Insufficient permissions') {
		super(message, 403);
	}
}

class NotFoundError extends AppError {
	constructor(message = 'Resource not found') {
		super(message, 404);
	}
}

class ConflictError extends AppError {
	constructor(message = 'Resource conflict') {
		super(message, 409);
	}
}

class RateLimitError extends AppError {
	constructor(message = 'Too many requests') {
		super(message, 429);
	}
}

module.exports = {
	errorHandler,
	asyncHandler,
	AppError,
	ValidationError,
	AuthenticationError,
	AuthorizationError,
	NotFoundError,
	ConflictError,
	RateLimitError,
};
