/**
 * Request Logger Middleware
 * Logs HTTP requests with performance metrics and structured data
 */

const logger = require('../utils/logger');

const requestLogger = (req, res, next) => {
	// Capture start time
	const startTime = Date.now();

	// Capture original end method
	const originalEnd = res.end;

	// Override end method to capture response data
	res.end = function(chunk, encoding) {
		// Calculate response time
		const responseTime = Date.now() - startTime;

		// Log the request
		logger.logAPICall(
			req.method,
			req.originalUrl,
			res.statusCode,
			responseTime,
			req.user?.id || null
		);

		// Call original end method
		originalEnd.call(this, chunk, encoding);
	};

	next();
};

module.exports = { requestLogger };
