var config = require('../config.js');

module.exports = exports = {
	getSensorImg: function (sensor) {
		if (sensor.img != null) { return sensor.img; }
		return 'img/missing.svg';
	},
		
	formatVal: function (sensor, val) {
		return val.toFixed();
	}
};