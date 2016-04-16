var exec = require('child_process').exec;
var config = require('../config.js');

module.exports = exports = {
	getSensorImg: function (sensor) {
		if (sensor.img != null) { return sensor.img; }
		return 'img/missing.svg';
	},
		
	formatVal: function (sensor, val) {
		if (log.trace())
			log.trace('Transforming value ' + val);
		
		return val.toFixed();
	},
	
	getConnChangedTime: function (timestamp) {
		var diff = new Date().getTime() - timestamp;
		return (diff / 1000).toFixed() + 's';
	},
	
	ping: function (host, callback) {
		exec('ping -c 2 -q ' + host, function (e, stdout, stderr) {
			callback(e == null);
		});
	}
};