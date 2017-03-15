var exec = require('child_process').exec;
var config = require('../config.js');

var HOURS_PER_DAY = 24;
var DAYS_PER_MONTH = 30;

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

	formatConnTime: function (timestamp) {
		var diff = new Date().getTime() - timestamp;

		var seconds = Math.floor(diff / 1000);
		var minutes = Math.floor(seconds / 60);
		var hours = Math.floor(minutes / 60);
		var days = Math.floor(hours / HOURS_PER_DAY);
		var months = Math.floor(days / DAYS_PER_MONTH);

		if (months != 0) {
			return months + 'M' + (days != 0 ? Math.floor(days % DAYS_PER_MONTH) + 'd' : '');
		}
		else if (days != 0) {
			return days + 'd' + (hours != 0 ? Math.floor(hours % HOURS_PER_DAY) + 'h' : '');
		}
		else if (hours != 0) {
			return hours + 'h';
		}
		else if (minutes != 0) {
			return minutes + 'm';
		}
		else {
			return seconds + 's';
		}
	},

	ping: function (host, callback) {
		exec('ping -c 2 -q ' + host, function (e, stdout, stderr) {
			callback(e == null);
		});
	}
};
