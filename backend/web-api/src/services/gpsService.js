const logger = require('../utils/logger');

class GPSService {
	constructor() {
		this.currentPosition = null;
		this.activeRoute = null;
		this.routes = new Map();
		this.waypoints = new Map();
		this.stats = {
			totalRoutes: 0,
			totalWaypoints: 0,
			totalDistance: 0,
			lastUpdate: null
		};
	}

	async getCurrentPosition() {
		try {
			return this.currentPosition;
		} catch (error) {
			logger.error('Error getting current GPS position:', error);
			throw error;
		}
	}

	async updatePosition(position) {
		try {
			const { latitude, longitude, altitude, speed, heading, timestamp = new Date() } = position;

			if (latitude === undefined || longitude === undefined) {
				throw new Error('Latitude and longitude are required');
			}

			this.currentPosition = {
				latitude: parseFloat(latitude),
				longitude: parseFloat(longitude),
				altitude: altitude ? parseFloat(altitude) : null,
				speed: speed ? parseFloat(speed) : null,
				heading: heading ? parseFloat(heading) : null,
				timestamp: new Date(timestamp),
				updatedAt: new Date()
			};

			// Add waypoint to active route if exists
			if (this.activeRoute) {
				await this.addWaypoint(this.activeRoute.id, this.currentPosition);
			}

			this.stats.lastUpdate = new Date();
			logger.logGPSData('GPS position updated', { latitude, longitude, altitude, speed });
			return this.currentPosition;
		} catch (error) {
			logger.error('Error updating GPS position:', error);
			throw error;
		}
	}

	async startRoute(metadata = {}) {
		try {
			if (this.activeRoute) {
				throw new Error('Route already active');
			}

			const route = {
				id: this._generateId(),
				startTime: new Date(),
				endTime: null,
				startPosition: this.currentPosition,
				endPosition: null,
				totalDistance: 0,
				totalDuration: 0,
				waypointCount: 0,
				averageSpeed: 0,
				maxSpeed: 0,
				metadata,
				createdAt: new Date()
			};

			this.activeRoute = route;
			this.routes.set(route.id, route);
			this.stats.totalRoutes++;

			logger.logGPSData('Route started', { routeId: route.id, startTime: route.startTime });
			return route;
		} catch (error) {
			logger.error('Error starting route:', error);
			throw error;
		}
	}

	async endRoute() {
		try {
			if (!this.activeRoute) {
				throw new Error('No active route');
			}

			const route = this.activeRoute;
			route.endTime = new Date();
			route.endPosition = this.currentPosition;
			route.totalDuration = route.endTime - route.startTime;

			// Calculate route statistics
			await this._calculateRouteStats(route);

			this.activeRoute = null;
			this.routes.set(route.id, route);

			logger.logGPSData('Route ended', {
				routeId: route.id,
				duration: route.totalDuration,
				distance: route.totalDistance
			});
			return route;
		} catch (error) {
			logger.error('Error ending route:', error);
			throw error;
		}
	}

	async addWaypoint(routeId, position) {
		try {
			const route = this.routes.get(routeId);
			if (!route) {
				throw new Error('Route not found');
			}

			const waypoint = {
				id: this._generateId(),
				routeId,
				latitude: position.latitude,
				longitude: position.longitude,
				altitude: position.altitude,
				speed: position.speed,
				heading: position.heading,
				timestamp: position.timestamp,
				createdAt: new Date()
			};

			this.waypoints.set(waypoint.id, waypoint);
			route.waypointCount++;
			this.stats.totalWaypoints++;

			// Update route distance
			if (route.waypoints && route.waypoints.length > 0) {
				const lastWaypoint = route.waypoints[route.waypoints.length - 1];
				const distance = this._calculateDistance(
					lastWaypoint.latitude, lastWaypoint.longitude,
					position.latitude, position.longitude
				);
				route.totalDistance += distance;
				this.stats.totalDistance += distance;
			}

			// Store waypoints in route for easy access
			if (!route.waypoints) route.waypoints = [];
			route.waypoints.push(waypoint);

			logger.logGPSData('Waypoint added', { routeId, waypointId: waypoint.id });
			return waypoint;
		} catch (error) {
			logger.error('Error adding waypoint:', error);
			throw error;
		}
	}

	async getRoutes(filters = {}) {
		try {
			let routes = Array.from(this.routes.values());

			// Apply filters
			if (filters.startDate) {
				const startDate = new Date(filters.startDate);
				routes = routes.filter(route => route.startTime >= startDate);
			}

			if (filters.endDate) {
				const endDate = new Date(filters.endDate);
				routes = routes.filter(route => route.startTime <= endDate);
			}

			if (filters.minDistance) {
				routes = routes.filter(route => route.totalDistance >= filters.minDistance);
			}

			if (filters.maxDistance) {
				routes = routes.filter(route => route.totalDistance <= filters.maxDistance);
			}

			// Sort by start time (newest first)
			routes.sort((a, b) => b.startTime - a.startTime);

			return routes;
		} catch (error) {
			logger.error('Error getting routes:', error);
			throw error;
		}
	}

	async getRoute(routeId) {
		try {
			const route = this.routes.get(routeId);
			if (!route) {
				throw new Error('Route not found');
			}

			// Get waypoints for this route
			const routeWaypoints = Array.from(this.waypoints.values())
				.filter(wp => wp.routeId === routeId)
				.sort((a, b) => a.timestamp - b.timestamp);

			return {
				...route,
				waypoints: routeWaypoints
			};
		} catch (error) {
			logger.error('Error getting route:', error);
			throw error;
		}
	}

	async exportRoute(routeId, format = 'json') {
		try {
			const route = await this.getRoute(routeId);

			switch (format.toLowerCase()) {
				case 'json':
					return JSON.stringify(route, null, 2);
				case 'gpx':
					return this._exportToGPX(route);
				case 'kml':
					return this._exportToKML(route);
				case 'csv':
					return this._exportToCSV(route);
				default:
					throw new Error('Unsupported export format');
			}
		} catch (error) {
			logger.error('Error exporting route:', error);
			throw error;
		}
	}

	async getGPSStats() {
		try {
			const stats = {
				...this.stats,
				currentPosition: this.currentPosition,
				activeRoute: this.activeRoute ? {
					id: this.activeRoute.id,
					startTime: this.activeRoute.startTime,
					duration: Date.now() - this.activeRoute.startTime,
					waypointCount: this.activeRoute.waypointCount
				} : null
			};

			return stats;
		} catch (error) {
			logger.error('Error getting GPS stats:', error);
			throw error;
		}
	}

	async _calculateRouteStats(route) {
		try {
			if (!route.waypoints || route.waypoints.length === 0) return;

			const speeds = route.waypoints
				.map(wp => wp.speed)
				.filter(speed => speed !== null && speed > 0);

			if (speeds.length > 0) {
				route.averageSpeed = speeds.reduce((sum, speed) => sum + speed, 0) / speeds.length;
				route.maxSpeed = Math.max(...speeds);
			}
		} catch (error) {
			logger.error('Error calculating route stats:', error);
		}
	}

	_calculateDistance(lat1, lon1, lat2, lon2) {
		// Haversine formula for calculating distance between two points
		const R = 6371; // Earth's radius in kilometers
		const dLat = (lat2 - lat1) * Math.PI / 180;
		const dLon = (lon2 - lon1) * Math.PI / 180;
		const a = Math.sin(dLat / 2) * Math.sin(dLat / 2) +
			Math.cos(lat1 * Math.PI / 180) * Math.cos(lat2 * Math.PI / 180) *
			Math.sin(dLon / 2) * Math.sin(dLon / 2);
		const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
		return R * c;
	}

	_generateId() {
		return Date.now().toString(36) + Math.random().toString(36).substr(2);
	}

	_exportToGPX(route) {
		let gpx = '<?xml version="1.0" encoding="UTF-8"?>\n';
		gpx += '<gpx version="1.1" creator="Jeep Sensor Hub">\n';
		gpx += `  <metadata><name>Route ${route.id}</name></metadata>\n`;
		gpx += '  <trk><name>Route</name><trkseg>\n';

		route.waypoints.forEach(wp => {
			gpx += `    <trkpt lat="${wp.latitude}" lon="${wp.longitude}">\n`;
			if (wp.altitude) gpx += `      <ele>${wp.altitude}</ele>\n`;
			gpx += `      <time>${wp.timestamp.toISOString()}</time>\n`;
			gpx += '    </trkpt>\n';
		});

		gpx += '  </trkseg></trk>\n</gpx>';
		return gpx;
	}

	_exportToKML(route) {
		let kml = '<?xml version="1.0" encoding="UTF-8"?>\n';
		kml += '<kml xmlns="http://www.opengis.net/kml/2.2">\n';
		kml += '  <Document>\n';
		kml += `    <name>Route ${route.id}</name>\n`;
		kml += '    <Placemark>\n';
		kml += '      <name>Route</name>\n';
		kml += '      <LineString>\n';
		kml += '        <coordinates>\n';

		route.waypoints.forEach(wp => {
			kml += `          ${wp.longitude},${wp.latitude}`;
			if (wp.altitude) kml += `,${wp.altitude}`;
			kml += '\n';
		});

		kml += '        </coordinates>\n';
		kml += '      </LineString>\n';
		kml += '    </Placemark>\n';
		kml += '  </Document>\n</kml>';
		return kml;
	}

	_exportToCSV(route) {
		let csv = 'Timestamp,Latitude,Longitude,Altitude,Speed,Heading\n';

		route.waypoints.forEach(wp => {
			csv += `${wp.timestamp.toISOString()},${wp.latitude},${wp.longitude}`;
			csv += `,${wp.altitude || ''},${wp.speed || ''},${wp.heading || ''}\n`;
		});

		return csv;
	}

	async cleanup() {
		try {
			// Clean up old routes (keep last 100)
			if (this.routes.size > 100) {
				const sortedRoutes = Array.from(this.routes.values())
					.sort((a, b) => b.startTime - a.startTime)
					.slice(0, 100);

				this.routes = new Map(
					sortedRoutes.map(route => [route.id, route])
				);
			}

			// Clean up old waypoints (keep last 10000)
			if (this.waypoints.size > 10000) {
				const sortedWaypoints = Array.from(this.waypoints.values())
					.sort((a, b) => b.timestamp - a.timestamp)
					.slice(0, 10000);

				this.waypoints = new Map(
					sortedWaypoints.map(wp => [wp.id, wp])
				);
			}

			logger.logGPSData('GPS service cleanup completed');
		} catch (error) {
			logger.error('Error during GPS service cleanup:', error);
		}
	}
}

module.exports = GPSService;
