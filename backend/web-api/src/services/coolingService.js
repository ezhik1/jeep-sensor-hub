const logger = require('../utils/logger');

class CoolingService {
	constructor() {
		this.coolingStatus = {
			engineTemp: 0,
			coolantTemp: 0,
			oilTemp: 0,
			ambientTemp: 0,
			fanSpeed: 0, // 0 = off, 1 = low, 2 = high
			fanOverride: false,
			acState: false,
			engineLoad: 0, // 0-100%
			lastUpdate: null
		};

		this.config = {
			lowSpeedThreshold: 85, // °C
			highSpeedThreshold: 95, // °C
			criticalThreshold: 105, // °C
			hysteresis: 5, // °C
			fanDelay: 2000, // ms
			overrideTimeout: 30000, // ms
			enabled: true
		};

		this.history = new Map();
		this.alerts = new Map();
		this.stats = {
			totalReadings: 0,
			peakTemp: 0,
			averageTemp: 0,
			fanRuntime: 0,
			lastUpdate: null
		};

		this._overrideTimer = null;
		this._lastFanState = 0;
	}

	async getCoolingStatus() {
		try {
			return this.coolingStatus;
		} catch (error) {
			logger.error('Error getting cooling status:', error);
			throw error;
		}
	}

	async updateCoolingStatus(status) {
		try {
			const { engineTemp, coolantTemp, oilTemp, ambientTemp, engineLoad, acState } = status;

			// Validate required fields
			if (engineTemp === undefined || coolantTemp === undefined) {
				throw new Error('Engine temperature and coolant temperature are required');
			}

			// Update cooling status
			this.coolingStatus = {
				...this.coolingStatus,
				engineTemp: parseFloat(engineTemp),
				coolantTemp: parseFloat(coolantTemp),
				oilTemp: oilTemp !== undefined ? parseFloat(oilTemp) : this.coolingStatus.oilTemp,
				ambientTemp: ambientTemp !== undefined ? parseFloat(ambientTemp) : this.coolingStatus.ambientTemp,
				engineLoad: engineLoad !== undefined ? parseFloat(engineLoad) : this.coolingStatus.engineLoad,
				acState: acState !== undefined ? Boolean(acState) : this.coolingStatus.acState,
				lastUpdate: new Date()
			};

			// Determine fan speed based on temperature and configuration
			await this._determineFanSpeed();

			// Store history
			await this._storeHistory();

			// Check for alerts
			await this._checkCoolingAlerts();

			// Update stats
			this._updateStats();

			logger.logCoolingEvent('Cooling status updated', {
				engineTemp,
				coolantTemp,
				fanSpeed: this.coolingStatus.fanSpeed
			});

			return this.coolingStatus;
		} catch (error) {
			logger.error('Error updating cooling status:', error);
			throw error;
		}
	}

	async getConfiguration() {
		try {
			return { ...this.config };
		} catch (error) {
			logger.error('Error getting cooling configuration:', error);
			throw error;
		}
	}

	async updateConfiguration(newConfig) {
		try {
			// Validate configuration
			const validConfig = this._validateConfiguration(newConfig);

			// Update configuration
			this.config = {
				...this.config,
				...validConfig
			};

			logger.logCoolingEvent('Cooling configuration updated', validConfig);
			return this.config;
		} catch (error) {
			logger.error('Error updating cooling configuration:', error);
			throw error;
		}
	}

	async setFanOverride(override, timeout = null) {
		try {
			if (typeof override !== 'boolean') {
				throw new Error('Fan override must be a boolean value');
			}

			// Clear existing override timer
			if (this._overrideTimer) {
				clearTimeout(this._overrideTimer);
				this._overrideTimer = null;
			}

			this.coolingStatus.fanOverride = override;
			this.coolingStatus.lastUpdate = new Date();

			// Set override timeout if specified
			if (override && timeout) {
				this._overrideTimer = setTimeout(() => {
					this.coolingStatus.fanOverride = false;
					this.coolingStatus.lastUpdate = new Date();
					logger.logCoolingEvent('Fan override timeout expired');
				}, timeout);
			}

			// Recalculate fan speed
			await this._determineFanSpeed();

			logger.logCoolingEvent('Fan override set', { override, timeout });
			return this.coolingStatus;
		} catch (error) {
			logger.error('Error setting fan override:', error);
			throw error;
		}
	}

	async setACState(acState) {
		try {
			if (typeof acState !== 'boolean') {
				throw new Error('AC state must be a boolean value');
			}

			this.coolingStatus.acState = acState;
			this.coolingStatus.lastUpdate = new Date();

			// Recalculate fan speed (AC state affects cooling requirements)
			await this._determineFanSpeed();

			logger.logCoolingEvent('AC state updated', { acState });
			return this.coolingStatus;
		} catch (error) {
			logger.error('Error setting AC state:', error);
			throw error;
		}
	}

	async setEngineLoad(engineLoad) {
		try {
			const load = parseFloat(engineLoad);
			if (isNaN(load) || load < 0 || load > 100) {
				throw new Error('Engine load must be a number between 0 and 100');
			}

			this.coolingStatus.engineLoad = load;
			this.coolingStatus.lastUpdate = new Date();

			// Recalculate fan speed (engine load affects cooling requirements)
			await this._determineFanSpeed();

			logger.logCoolingEvent('Engine load updated', { engineLoad: load });
			return this.coolingStatus;
		} catch (error) {
			logger.error('Error setting engine load:', error);
			throw error;
		}
	}

	async setAmbientTemperature(ambientTemp) {
		try {
			const temp = parseFloat(ambientTemp);
			if (isNaN(temp)) {
				throw new Error('Ambient temperature must be a valid number');
			}

			this.coolingStatus.ambientTemp = temp;
			this.coolingStatus.lastUpdate = new Date();

			// Recalculate fan speed (ambient temperature affects cooling efficiency)
			await this._determineFanSpeed();

			logger.logCoolingEvent('Ambient temperature updated', { ambientTemp: temp });
			return this.coolingStatus;
		} catch (error) {
			logger.error('Error setting ambient temperature:', error);
			throw error;
		}
	}

	async getCoolingHistory(filters = {}) {
		try {
			let history = Array.from(this.history.values());

			// Apply filters
			if (filters.startDate) {
				const startDate = new Date(filters.startDate);
				history = history.filter(entry => entry.timestamp >= startDate);
			}

			if (filters.endDate) {
				const endDate = new Date(filters.endDate);
				history = history.filter(entry => entry.timestamp <= endDate);
			}

			if (filters.minTemp) {
				history = history.filter(entry => entry.engineTemp >= filters.minTemp);
			}

			if (filters.maxTemp) {
				history = history.filter(entry => entry.engineTemp <= filters.maxTemp);
			}

			if (filters.fanSpeed !== undefined) {
				history = history.filter(entry => entry.fanSpeed === filters.fanSpeed);
			}

			// Sort by timestamp (newest first)
			history.sort((a, b) => b.timestamp - a.timestamp);

			// Apply limit
			if (filters.limit) {
				history = history.slice(0, filters.limit);
			}

			return history;
		} catch (error) {
			logger.error('Error getting cooling history:', error);
			throw error;
		}
	}

	async resetHistory() {
		try {
			this.history.clear();
			this.stats = {
				totalReadings: 0,
				peakTemp: 0,
				averageTemp: 0,
				fanRuntime: 0,
				lastUpdate: null
			};

			logger.logCoolingEvent('Cooling history reset');
			return { message: 'Cooling history has been reset' };
		} catch (error) {
			logger.error('Error resetting cooling history:', error);
			throw error;
		}
	}

	async simulateFault(faultType) {
		try {
			const validFaults = ['sensor_failure', 'fan_failure', 'pump_failure', 'thermostat_failure'];

			if (!validFaults.includes(faultType)) {
				throw new Error(`Invalid fault type. Must be one of: ${validFaults.join(', ')}`);
			}

			// Simulate fault behavior
			switch (faultType) {
				case 'sensor_failure':
					this.coolingStatus.engineTemp = 999; // Invalid reading
					break;
				case 'fan_failure':
					this.coolingStatus.fanSpeed = 0;
					break;
				case 'pump_failure':
					this.coolingStatus.coolantTemp = this.coolingStatus.engineTemp + 20; // Overheating
					break;
				case 'thermostat_failure':
					this.coolingStatus.coolantTemp = 50; // Stuck open
					break;
			}

			logger.logCoolingEvent('Fault simulated', { faultType });
			return {
				message: `Fault ${faultType} has been simulated`,
				faultType,
				status: this.coolingStatus
			};
		} catch (error) {
			logger.error('Error simulating fault:', error);
			throw error;
		}
	}

	async _determineFanSpeed() {
		try {
			if (!this.config.enabled) {
				this.coolingStatus.fanSpeed = 0;
				return;
			}

			// Check if override is active
			if (this.coolingStatus.fanOverride) {
				this.coolingStatus.fanSpeed = 2; // High speed
				return;
			}

			const { engineTemp, coolantTemp, acState, engineLoad } = this.coolingStatus;
			const { lowSpeedThreshold, highSpeedThreshold, hysteresis } = this.config;

			// Determine base fan speed from temperature
			let targetSpeed = 0;

			if (coolantTemp >= highSpeedThreshold + hysteresis) {
				targetSpeed = 2; // High speed
			} else if (coolantTemp >= lowSpeedThreshold + hysteresis) {
				targetSpeed = 1; // Low speed
			} else if (coolantTemp < lowSpeedThreshold - hysteresis) {
				targetSpeed = 0; // Off
			} else {
				// Keep current speed (hysteresis)
				targetSpeed = this.coolingStatus.fanSpeed;
			}

			// Adjust for AC state
			if (acState && targetSpeed === 0) {
				targetSpeed = 1; // Low speed when AC is on
			}

			// Adjust for engine load
			if (engineLoad > 80 && targetSpeed < 2) {
				targetSpeed = Math.max(targetSpeed, 1); // At least low speed under high load
			}

			// Apply fan delay for state changes
			if (targetSpeed !== this._lastFanState) {
				setTimeout(() => {
					this.coolingStatus.fanSpeed = targetSpeed;
					this.coolingStatus.lastUpdate = new Date();
					logger.logCoolingEvent('Fan speed changed', {
						from: this._lastFanState,
						to: targetSpeed
					});
				}, this.config.fanDelay);
			}

			this._lastFanState = targetSpeed;
		} catch (error) {
			logger.error('Error determining fan speed:', error);
		}
	}

	async _storeHistory() {
		try {
			const entry = {
				id: this._generateId(),
				...this.coolingStatus,
				timestamp: new Date(),
				createdAt: new Date()
			};

			this.history.set(entry.id, entry);

			// Clean up old history (keep last 10000 entries)
			if (this.history.size > 10000) {
				const sortedEntries = Array.from(this.history.values())
					.sort((a, b) => b.timestamp - a.timestamp)
					.slice(0, 5000);

				this.history = new Map(
					sortedEntries.map(entry => [entry.id, entry])
				);
			}
		} catch (error) {
			logger.error('Error storing cooling history:', error);
		}
	}

	async _checkCoolingAlerts() {
		try {
			const { engineTemp, coolantTemp, fanSpeed } = this.coolingStatus;
			const { criticalThreshold } = this.config;

			// Check critical temperature
			if (engineTemp > criticalThreshold || coolantTemp > criticalThreshold) {
				await this._createAlert('critical_temperature', 'critical',
					`Critical temperature detected: Engine ${engineTemp}°C, Coolant ${coolantTemp}°C`);
			}

			// Check fan failure
			if (engineTemp > this.config.highSpeedThreshold && fanSpeed === 0) {
				await this._createAlert('fan_failure', 'critical',
					`Fan failure detected: Engine temp ${engineTemp}°C but fan is off`);
			}

			// Check sensor failure
			if (engineTemp > 200 || coolantTemp > 200) {
				await this._createAlert('sensor_failure', 'warning',
					`Possible sensor failure: Unrealistic temperature readings`);
			}
		} catch (error) {
			logger.error('Error checking cooling alerts:', error);
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
			logger.logCoolingEvent('Cooling alert created', { alertId: alert.id, type, level, message });
		} catch (error) {
			logger.error('Error creating cooling alert:', error);
		}
	}

	_validateConfiguration(config) {
		try {
			const validated = {};

			if (config.lowSpeedThreshold !== undefined) {
				const threshold = parseFloat(config.lowSpeedThreshold);
				if (isNaN(threshold) || threshold < 50 || threshold > 120) {
					throw new Error('Low speed threshold must be between 50 and 120°C');
				}
				validated.lowSpeedThreshold = threshold;
			}

			if (config.highSpeedThreshold !== undefined) {
				const threshold = parseFloat(config.highSpeedThreshold);
				if (isNaN(threshold) || threshold < 80 || threshold > 130) {
					throw new Error('High speed threshold must be between 80 and 130°C');
				}
				validated.highSpeedThreshold = threshold;
			}

			if (config.criticalThreshold !== undefined) {
				const threshold = parseFloat(config.criticalThreshold);
				if (isNaN(threshold) || threshold < 100 || threshold > 150) {
					throw new Error('Critical threshold must be between 100 and 150°C');
				}
				validated.criticalThreshold = threshold;
			}

			if (config.hysteresis !== undefined) {
				const hysteresis = parseFloat(config.hysteresis);
				if (isNaN(hysteresis) || hysteresis < 1 || hysteresis > 10) {
					throw new Error('Hysteresis must be between 1 and 10°C');
				}
				validated.hysteresis = hysteresis;
			}

			if (config.fanDelay !== undefined) {
				const delay = parseInt(config.fanDelay);
				if (isNaN(delay) || delay < 0 || delay > 10000) {
					throw new Error('Fan delay must be between 0 and 10000ms');
				}
				validated.fanDelay = delay;
			}

			if (config.overrideTimeout !== undefined) {
				const timeout = parseInt(config.overrideTimeout);
				if (isNaN(timeout) || timeout < 5000 || timeout > 300000) {
					throw new Error('Override timeout must be between 5000 and 300000ms');
				}
				validated.overrideTimeout = timeout;
			}

			if (config.enabled !== undefined) {
				if (typeof config.enabled !== 'boolean') {
					throw new Error('Enabled must be a boolean value');
				}
				validated.enabled = config.enabled;
			}

			return validated;
		} catch (error) {
			logger.error('Error validating cooling configuration:', error);
			throw error;
		}
	}

	_updateStats() {
		try {
			const { engineTemp, fanSpeed } = this.coolingStatus;

			this.stats.totalReadings++;
			this.stats.peakTemp = Math.max(this.stats.peakTemp, engineTemp);
			this.stats.lastUpdate = new Date();

			// Update average temperature
			if (this.stats.totalReadings === 1) {
				this.stats.averageTemp = engineTemp;
			} else {
				this.stats.averageTemp = (this.stats.averageTemp * (this.stats.totalReadings - 1) + engineTemp) / this.stats.totalReadings;
			}

			// Update fan runtime
			if (fanSpeed > 0) {
				this.stats.fanRuntime += 1; // Assuming 1-second intervals
			}
		} catch (error) {
			logger.error('Error updating cooling stats:', error);
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

			// Clear override timer
			if (this._overrideTimer) {
				clearTimeout(this._overrideTimer);
				this._overrideTimer = null;
			}

			logger.logCoolingEvent('Cooling service cleanup completed');
		} catch (error) {
			logger.error('Error during cooling service cleanup:', error);
		}
	}
}

module.exports = CoolingService;
