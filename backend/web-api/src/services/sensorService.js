const logger = require('../utils/logger');

class SensorService {
	constructor() {
		this.sensorData = new Map();
		this.alerts = new Map();
		this.stats = {
			totalReadings: 0,
			lastUpdate: null,
			sensorTypes: new Set()
		};
	}

	async getSensorData(type = null) {
		try {
			if (type) {
				const data = this.sensorData.get(type);
				return data ? Array.from(data.values()) : [];
			}

			const allData = {};
			for (const [sensorType, readings] of this.sensorData) {
				allData[sensorType] = Array.from(readings.values());
			}
			return allData;
		} catch (error) {
			logger.error('Error getting sensor data:', error);
			throw error;
		}
	}

	async submitSensorData(data) {
		try {
			const { type, value, timestamp = new Date(), metadata = {} } = data;

			if (!type || value === undefined) {
				throw new Error('Sensor type and value are required');
			}

			const sensorReading = {
				id: this._generateId(),
				type,
				value,
				timestamp: new Date(timestamp),
				metadata,
				createdAt: new Date()
			};

			if (!this.sensorData.has(type)) {
				this.sensorData.set(type, new Map());
			}

			this.sensorData.get(type).set(sensorReading.id, sensorReading);
			this.stats.totalReadings++;
			this.stats.lastUpdate = new Date();
			this.stats.sensorTypes.add(type);

			// Check for alerts
			await this._checkAlerts(sensorReading);

			logger.logSensorData('Sensor data submitted', { type, value, timestamp });
			return sensorReading;
		} catch (error) {
			logger.error('Error submitting sensor data:', error);
			throw error;
		}
	}

	async getSensorStats() {
		try {
			const stats = {
				...this.stats,
				sensorTypes: Array.from(this.stats.sensorTypes),
				sensorCounts: {}
			};

			for (const [type, readings] of this.sensorData) {
				stats.sensorCounts[type] = readings.size;
			}

			return stats;
		} catch (error) {
			logger.error('Error getting sensor stats:', error);
			throw error;
		}
	}

	async getAlerts() {
		try {
			return Array.from(this.alerts.values());
		} catch (error) {
			logger.error('Error getting alerts:', error);
			throw error;
		}
	}

	async acknowledgeAlert(alertId) {
		try {
			const alert = this.alerts.get(alertId);
			if (!alert) {
				throw new Error('Alert not found');
			}

			alert.acknowledged = true;
			alert.acknowledgedAt = new Date();
			this.alerts.set(alertId, alert);

			logger.logSensorData('Alert acknowledged', { alertId, type: alert.type });
			return alert;
		} catch (error) {
			logger.error('Error acknowledging alert:', error);
			throw error;
		}
	}

	async _checkAlerts(reading) {
		try {
			const { type, value } = reading;

			// Define alert thresholds (in production, these would come from config)
			const thresholds = {
				temperature: { min: -40, max: 120 },
				humidity: { min: 0, max: 100 },
				pressure: { min: 0, max: 50 },
				voltage: { min: 10, max: 15 },
				current: { min: -100, max: 100 }
			};

			const threshold = thresholds[type];
			if (!threshold) return;

			let alertLevel = null;
			let message = '';

			if (value < threshold.min) {
				alertLevel = 'warning';
				message = `${type} value ${value} is below minimum threshold ${threshold.min}`;
			} else if (value > threshold.max) {
				alertLevel = 'critical';
				message = `${type} value ${value} is above maximum threshold ${threshold.max}`;
			}

			if (alertLevel) {
				const alert = {
					id: this._generateId(),
					type,
					level: alertLevel,
					message,
					value,
					timestamp: new Date(),
					acknowledged: false,
					createdAt: new Date()
				};

				this.alerts.set(alert.id, alert);
				logger.logSensorData('Alert created', { alertId: alert.id, type, level, message });
			}
		} catch (error) {
			logger.error('Error checking alerts:', error);
		}
	}

	_generateId() {
		return Date.now().toString(36) + Math.random().toString(36).substr(2);
	}

	async cleanup() {
		try {
			// Clean up old data (keep last 1000 readings per sensor type)
			for (const [type, readings] of this.sensorData) {
				if (readings.size > 1000) {
					const sortedReadings = Array.from(readings.values())
						.sort((a, b) => b.timestamp - a.timestamp)
						.slice(0, 1000);

					this.sensorData.set(type, new Map(
						sortedReadings.map(reading => [reading.id, reading])
					));
				}
			}

			// Clean up old alerts (keep last 100)
			if (this.alerts.size > 100) {
				const sortedAlerts = Array.from(this.alerts.values())
					.sort((a, b) => b.timestamp - a.timestamp)
					.slice(0, 100);

				this.alerts = new Map(
					sortedAlerts.map(alert => [alert.id, alert])
				);
			}

			logger.logSensorData('Sensor service cleanup completed');
		} catch (error) {
			logger.error('Error during sensor service cleanup:', error);
		}
	}
}

module.exports = SensorService;
