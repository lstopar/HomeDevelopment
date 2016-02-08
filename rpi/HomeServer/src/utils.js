var config = require('../config.js');

module.exports = exports = {
	getSensorImg: function (sensor) {
		switch (sensor.type) {
		case 'temperature':
			return 'img/temperature.png';
		case 'humidity':
			return 'img/humidity.png';
		case 'luminosity':
			return 'img/luminosity.png';
		case 'dimmer':
			return 'img/bulb.png';
		default:
			throw new Error('Unknown sensor type: ' + sensor.type);
		}
	},
	
	formatVal: function (sensor, val) {
		return val.toFixed();
	}
};