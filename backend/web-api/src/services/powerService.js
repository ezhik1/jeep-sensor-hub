const logger = require('../utils/logger');

class PowerService {
	constructor() {
		this.powerStatus = {
			source: 'auxiliary', // 'auxiliary', 'starter', 'unknown'
			voltage: 0,
			current: 0,
			power: 0,
			batteryLevel: 0,
			engineState: false,
			charging: false,
			lastUpdate: null
		};

		this.consumptionHistory = new Map();
		this.chargingHistory = new Map();
		this.alerts = new Map();
		this.stats = {
			totalConsumption: 0,
			totalCharging: 0,
			peakPower: 0,
			averageEfficiency: 0,
			lastUpdate: null
		};
	}

	async getPowerStatus() {
		try {
			return this.powerStatus;
		} catch (error) {
			logger.error('Error getting power status:', error);
			throw error;
		}
	}

	async updatePowerStatus(status) {
		try {
			const { voltage, current, batteryLevel, engineState, charging, source } = status;

			// Validate required fields
			if (voltage === undefined || current === undefined) {
				throw new Error('Voltage and current are required');
			}

			// Calculate power
			const power = voltage * current;

			// Update power status
			this.powerStatus = {
				...this.powerStatus,
				voltage: parseFloat(voltage),
				current: parseFloat(current),
				power: parseFloat(power),
				batteryLevel: batteryLevel !== undefined ? parseFloat(batteryLevel) : this.powerStatus.batteryLevel,
				engineState: engineState !== undefined ? Boolean(engineState) : this.powerStatus.engineState,
				charging: charging !== undefined ? Boolean(charging) : this.powerStatus.charging,
				source: source || this.powerStatus.source,
				lastUpdate: new Date()
			};

			// Store consumption data
			await this._storeConsumptionData();

			// Check for alerts
			await this._checkPowerAlerts();

			// Update stats
			this._updateStats();

			logger.logPowerEvent('Power status updated', { voltage, current, power, batteryLevel });
			return this.powerStatus;
		} catch (error) {
			logger.error('Error updating power status:', error);
			throw error;
		}
	}

	async getConsumptionHistory(filters = {}) {
		try {
			let history = Array.from(this.consumptionHistory.values());

			// Apply filters
			if (filters.startDate) {
				const startDate = new Date(filters.startDate);
				history = history.filter(entry => entry.timestamp >= startDate);
			}

			if (filters.endDate) {
				const endDate = new Date(filters.endDate);
				history = history.filter(entry => entry.timestamp <= endDate);
			}

			if (filters.minPower) {
				history = history.filter(entry => entry.power >= filters.minPower);
			}

			if (filters.maxPower) {
				history = history.filter(entry => entry.power <= filters.maxPower);
			}

			// Sort by timestamp (newest first)
			history.sort((a, b) => b.timestamp - a.timestamp);

			// Apply limit
			if (filters.limit) {
				history = history.slice(0, filters.limit);
			}

			return history;
		} catch (error) {
			logger.error('Error getting consumption history:', error);
			throw error;
		}
	}

	async getBatteryHealth() {
		try {
			const { voltage, batteryLevel, charging } = this.powerStatus;

			let health = 'unknown';
			let status = 'unknown';

			// Determine battery health based on voltage
			if (voltage >= 12.6) {
				health = 'excellent';
				status = 'fully_charged';
			} else if (voltage >= 12.4) {
				health = 'good';
				status = 'charged';
			} else if (voltage >= 12.2) {
				health = 'fair';
				status = 'partially_charged';
			} else if (voltage >= 12.0) {
				health = 'poor';
				status = 'discharged';
			} else {
				health = 'critical';
				status = 'deeply_discharged';
			}

			// Adjust status based on charging state
			if (charging) {
				status = 'charging';
			}

			return {
				health,
				status,
				voltage,
				batteryLevel,
				charging,
				lastUpdate: this.powerStatus.lastUpdate
			};
		} catch (error) {
			logger.error('Error getting battery health:', error);
			throw error;
		}
	}

	async switchPowerSource(source) {
		try {
			if (!['auxiliary', 'starter'].includes(source)) {
				throw new Error('Invalid power source. Must be "auxiliary" or "starter"');
			}

			const previousSource = this.powerStatus.source;
			this.powerStatus.source = source;
			this.powerStatus.lastUpdate = new Date();

			logger.logPowerEvent('Power source switched', {
				from: previousSource,
				to: source,
				timestamp: new Date()
			});

			return {
				previousSource,
				currentSource: source,
				timestamp: new Date()
			};
		} catch (error) {
			logger.error('Error switching power source:', error);
			throw error;
		}
	}

	async getPowerAlerts() {
		try {
			return Array.from(this.alerts.values());
		} catch (error) {
			logger.error('Error getting power alerts:', error);
			throw error;
		}
	}

	async getEfficiencyMetrics(filters = {}) {
		try {
			const history = await this.getConsumptionHistory(filters);

			if (history.length === 0) {
				return {
					averagePower: 0,
					peakPower: 0,
					totalEnergy: 0,
					efficiency: 0,
					period: filters
				};
			}

			const totalEnergy = history.reduce((sum, entry) => sum + entry.energy, 0);
			const averagePower = history.reduce((sum, entry) => sum + entry.power, 0) / history.length;
			const peakPower = Math.max(...history.map(entry => entry.power));

			// Calculate efficiency (simplified - in real system would consider charging efficiency)
			const efficiency = this.powerStatus.charging ? 85 : 95; // Percentage

			return {
				averagePower: parseFloat(averagePower.toFixed(2)),
				peakPower: parseFloat(peakPower.toFixed(2)),
				totalEnergy: parseFloat(totalEnergy.toFixed(2)),
				efficiency: parseFloat(efficiency.toFixed(2)),
				period: filters,
				dataPoints: history.length
			};
		} catch (error) {
			logger.error('Error getting efficiency metrics:', error);
			throw error;
		}
	}

	async getChargingHistory(filters = {}) {
		try {
			let history = Array.from(this.chargingHistory.values());

			// Apply filters
			if (filters.startDate) {
				const startDate = new Date(filters.startDate);
				history = history.filter(entry => entry.timestamp >= startDate);
			}

			if (filters.endDate) {
				const endDate = new Date(filters.endDate);
				history = history.filter(entry => entry.timestamp <= endDate);
			}

			if (filters.charging !== undefined) {
				history = history.filter(entry => entry.charging === filters.charging);
			}

			// Sort by timestamp (newest first)
			history.sort((a, b) => b.timestamp - a.timestamp);

			// Apply limit
			if (filters.limit) {
				history = history.slice(0, filters.limit);
			}

			return history;
		} catch (error) {
			logger.error('Error getting charging history:', error);
			throw error;
		}
	}

	async _storeConsumptionData() {
		try {
			const { voltage, current, power, timestamp = new Date() } = this.powerStatus;

			const entry = {
				id: this._generateId(),
				voltage,
				current,
				power,
				energy: power * (1/3600), // Convert to kWh (assuming 1-second intervals)
				timestamp: new Date(timestamp),
				source: this.powerStatus.source,
				engineState: this.powerStatus.engineState,
				charging: this.powerStatus.charging,
				createdAt: new Date()
			};

			this.consumptionHistory.set(entry.id, entry);

			// Store charging state change
			if (this._lastChargingState !== this.powerStatus.charging) {
				const chargingEntry = {
					id: this._generateId(),
					charging: this.powerStatus.charging,
					voltage,
					batteryLevel: this.powerStatus.batteryLevel,
					timestamp: new Date(),
					createdAt: new Date()
				};

				this.chargingHistory.set(chargingEntry.id, chargingEntry);
				this._lastChargingState = this.powerStatus.charging;
			}

			// Clean up old data
			if (this.consumptionHistory.size > 10000) {
				const sortedEntries = Array.from(this.consumptionHistory.values())
					.sort((a, b) => b.timestamp - a.timestamp)
					.slice(0, 5000);

				this.consumptionHistory = new Map(
					sortedEntries.map(entry => [entry.id, entry])
				);
			}
		} catch (error) {
			logger.error('Error storing consumption data:', error);
		}
	}

	async _checkPowerAlerts() {
		try {
			const { voltage, current, power, batteryLevel } = this.powerStatus;

			// Define alert thresholds
			const thresholds = {
				voltage: { min: 10.5, max: 15.0 },
				current: { min: -150, max: 150 },
				power: { max: 2000 },
				batteryLevel: { min: 10 }
			};

			// Check voltage alerts
			if (voltage < thresholds.voltage.min) {
				await this._createAlert('voltage_low', 'critical',
					`Battery voltage ${voltage}V is below critical threshold ${thresholds.voltage.min}V`);
			} else if (voltage > thresholds.voltage.max) {
				await this._createAlert('voltage_high', 'warning',
					`Battery voltage ${voltage}V is above threshold ${thresholds.voltage.max}V`);
			}

			// Check current alerts
			if (Math.abs(current) > thresholds.current.max) {
				await this._createAlert('current_high', 'warning',
					`Current draw ${current}A exceeds threshold ${thresholds.current.max}A`);
			}

			// Check power alerts
			if (power > thresholds.power.max) {
				await this._createAlert('power_high', 'warning',
					`Power consumption ${power}W exceeds threshold ${thresholds.power.max}W`);
			}

			// Check battery level alerts
			if (batteryLevel < thresholds.batteryLevel.min) {
				await this._createAlert('battery_low', 'critical',
					`Battery level ${batteryLevel}% is below critical threshold ${thresholds.batteryLevel.min}%`);
			}
		} catch (error) {
			logger.error('Error checking power alerts:', error);
		}
	}

	async _createAlert(type, level, message) {
		try {
			const alert = {
				id: this._generateId(),
				type,
				level,
				message,
				timestamp: new Date(),
				acknowledged: false,
				createdAt: new Date()
			};

			this.alerts.set(alert.id, alert);
			logger.logPowerEvent('Power alert created', { alertId: alert.id, type, level, message });
		} catch (error) {
			logger.error('Error creating power alert:', error);
		}
	}

	_updateStats() {
		try {
			const { power } = this.powerStatus;

			this.stats.peakPower = Math.max(this.stats.peakPower, power);
			this.stats.lastUpdate = new Date();
		} catch (error) {
			logger.error('Error updating power stats:', error);
		}
	}

	_generateId() {
		return Date.now().toString(36) + Math.random().toString(36).substr(2);
	}

	async cleanup() {
		try {
			// Clean up old alerts (keep last 100)
			if (this.alerts.size > 100) {
				const sortedAlerts = Array.from(this.alerts.values())
					.sort((a, b) => b.timestamp - a.timestamp)
					.slice(0, 100);

				this.alerts = new Map(
					sortedAlerts.map(alert => [alert.id, alert])
				);
			}

			// Clean up old charging history (keep last 1000)
			if (this.chargingHistory.size > 1000) {
				const sortedCharging = Array.from(this.chargingHistory.values())
					.sort((a, b) => b.timestamp - a.timestamp)
					.slice(0, 1000);

				this.chargingHistory = new Map(
					sortedCharging.map(entry => [entry.id, entry])
				);
			}

			logger.logPowerEvent('Power service cleanup completed');
		} catch (error) {
			logger.error('Error during power service cleanup:', error);
		}
	}
}

module.exports = PowerService;
