/**
 * Authentication Middleware
 * Validates JWT tokens and provides user authentication
 */

const jwt = require('jsonwebtoken');
const { AuthenticationError, AuthorizationError } = require('./errorHandler');
const logger = require('../utils/logger');

const authMiddleware = async (req, res, next) => {
	try {
		// Get token from header
		const authHeader = req.headers.authorization;

		if (!authHeader || !authHeader.startsWith('Bearer ')) {
			throw new AuthenticationError('No token provided');
		}

		const token = authHeader.substring(7); // Remove 'Bearer ' prefix

		// Verify token
		const decoded = jwt.verify(token, process.env.JWT_SECRET || 'default-secret');

		// Add user info to request
		req.user = {
			id: decoded.id,
			email: decoded.email,
			role: decoded.role,
			permissions: decoded.permissions || [],
		};

		// Log successful authentication
		logger.info('User authenticated', {
			userId: req.user.id,
			email: req.user.email,
			endpoint: req.originalUrl,
			method: req.method,
		});

		next();
	} catch (error) {
		if (error.name === 'TokenExpiredError') {
			logger.warn('Token expired', {
				ip: req.ip,
				endpoint: req.originalUrl,
				userAgent: req.get('User-Agent'),
			});
			throw new AuthenticationError('Token expired');
		} else if (error.name === 'JsonWebTokenError') {
			logger.warn('Invalid token', {
				ip: req.ip,
				endpoint: req.originalUrl,
				userAgent: req.get('User-Agent'),
			});
			throw new AuthenticationError('Invalid token');
		} else {
			logger.error('Authentication error', {
				error: error.message,
				ip: req.ip,
				endpoint: req.originalUrl,
			});
			throw error;
		}
	}
};

// Role-based authorization middleware
const requireRole = (roles) => {
	return (req, res, next) => {
		if (!req.user) {
			throw new AuthenticationError('Authentication required');
		}

		const userRole = req.user.role;
		const allowedRoles = Array.isArray(roles) ? roles : [roles];

		if (!allowedRoles.includes(userRole)) {
			logger.warn('Insufficient role permissions', {
				userId: req.user.id,
				userRole,
				requiredRoles: allowedRoles,
				endpoint: req.originalUrl,
			});
			throw new AuthorizationError('Insufficient permissions');
		}

		next();
	};
};

// Permission-based authorization middleware
const requirePermission = (permissions) => {
	return (req, res, next) => {
		if (!req.user) {
			throw new AuthenticationError('Authentication required');
		}

		const userPermissions = req.user.permissions || [];
		const requiredPermissions = Array.isArray(permissions) ? permissions : [permissions];

		const hasAllPermissions = requiredPermissions.every(permission =>
			userPermissions.includes(permission)
		);

		if (!hasAllPermissions) {
			logger.warn('Insufficient permissions', {
				userId: req.user.id,
				userPermissions,
				requiredPermissions,
				endpoint: req.originalUrl,
			});
			throw new AuthorizationError('Insufficient permissions');
		}

		next();
	};
};

// Optional authentication middleware (doesn't fail if no token)
const optionalAuth = async (req, res, next) => {
	try {
		const authHeader = req.headers.authorization;

		if (authHeader && authHeader.startsWith('Bearer ')) {
			const token = authHeader.substring(7);
			const decoded = jwt.verify(token, process.env.JWT_SECRET || 'default-secret');

			req.user = {
				id: decoded.id,
				email: decoded.email,
				role: decoded.role,
				permissions: decoded.permissions || [],
			};
		}

		next();
	} catch (error) {
		// Continue without authentication
		next();
	}
};

module.exports = {
	authMiddleware,
	requireRole,
	requirePermission,
	optionalAuth,
};
