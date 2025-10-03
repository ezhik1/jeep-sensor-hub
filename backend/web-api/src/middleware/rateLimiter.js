/**
 * Rate Limiter Middleware
 * Prevents API abuse and ensures fair usage across all endpoints
 */

const { RateLimiterMemory } = require('rate-limiter-flexible');
const logger = require('../utils/logger');

// Create rate limiters for different endpoints
const generalLimiter = new RateLimiterMemory({
	keyGenerator: (req) => req.ip,
	points: 100, // Number of requests
	duration: 60, // Per 60 seconds
});

const authLimiter = new RateLimiterMemory({
	keyGenerator: (req) => req.ip,
	points: 5, // 5 login attempts
	duration: 300, // Per 5 minutes
});

const sensorDataLimiter = new RateLimiterMemory({
	keyGenerator: (req) => req.ip,
	points: 1000, // 1000 sensor data submissions
	duration: 60, // Per minute
});

const createRateLimiter = (limiter, endpoint) => {
	return async (req, res, next) => {
		try {
			await limiter.consume(req.ip);
			next();
		} catch (rejRes) {
			// Log rate limit violation
			logger.logSecurityEvent('rate_limit_exceeded', req.ip, req.get('User-Agent'), {
				endpoint,
				remainingPoints: rejRes.remainingPoints,
				msBeforeNext: rejRes.msBeforeNext,
			});

			// Set rate limit headers
			res.set({
				'Retry-After': Math.ceil(rejRes.msBeforeNext / 1000),
				'X-RateLimit-Limit': rejRes.totalPoints,
				'X-RateLimit-Remaining': rejRes.remainingPoints,
				'X-RateLimit-Reset': new Date(Date.now() + rejRes.msBeforeNext),
			});

			res.status(429).json({
				error: {
					message: 'Too many requests',
					type: 'RateLimitError',
					retryAfter: Math.ceil(rejRes.msBeforeNext / 1000),
				},
			});
		}
	};
};

// Apply rate limiting to different route groups
const rateLimiter = (req, res, next) => {
	// Determine which rate limiter to use based on the endpoint
	const path = req.path;

	if (path.startsWith('/api/auth')) {
		return createRateLimiter(authLimiter, 'auth')(req, res, next);
	} else if (path.startsWith('/api/sensors') && req.method === 'POST') {
		return createRateLimiter(sensorDataLimiter, 'sensor_data')(req, res, next);
	} else {
		return createRateLimiter(generalLimiter, 'general')(req, res, next);
	}
};

module.exports = { rateLimiter };
