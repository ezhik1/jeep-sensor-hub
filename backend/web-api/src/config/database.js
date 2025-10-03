const path = require('path');

module.exports = {
	development: {
		dialect: 'sqlite',
		storage: path.join(__dirname, '../../data/jeep-sensor-hub-dev.db'),
		logging: console.log,
		define: {
			timestamps: true,
			underscored: true
		}
	},
	test: {
		dialect: 'sqlite',
		storage: path.join(__dirname, '../../data/jeep-sensor-hub-test.db'),
		logging: false,
		define: {
			timestamps: true,
			underscored: true
		}
	},
	production: {
		dialect: 'sqlite',
		storage: path.join(__dirname, '../../data/jeep-sensor-hub.db'),
		logging: false,
		define: {
			timestamps: true,
			underscored: true
		}
	}
};
