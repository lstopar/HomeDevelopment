var config = require('../config.js');
var rpi = require('../' + config.rpilib);

var values = {};
var sensors = {
	'DHT11': {
		sensor: new rpi.DHT11(4),
		onRead: function (e, vals) {
			if (e != null) {
				log.error(e, 'Failed to read sensor!');
				return;
			}
			
			log.info('result: %s', JSON.stringify(vals));
		}
	},
	'YL-40': {
		sensor: new rpi.YL40Adc({
			inputs: [
			    {
			    	id: 'luminocity',
			    	number: 0
			    },
			    {
			    	id: 'temperature',
			    	number: 1
			    }
			]
		}),
		onRead: function (e, vals) {
			if (e != null) {
				log.error(e, 'Failed to read ADC!');
				return;
			}
			
			// transform the results
			vals.luminocity = (255 - vals.luminocity) / 255;
			
			log.info('Got output from ADC: %s', JSON.stringify(vals));
		}
	}
};

for (var sensorId in sensors) {
	if (sensors[sensorId].onRead == null) throw new Error('Sensor ' + sensorId + ' has no callback function!');
}

exports.read = function () {
	for (var sensorId in sensors) {
		var config = sensors[sensorId];
		var sensor = config.sensor;
		var callback = config.onRead;
		
		sensor.read(callback);
	}
}

exports.init = function () {
	log.info('Initializing sensors ...');
	
	for (var sensorId in sensors) {
		sensors[sensorId].sensor.init();
	}
	
	log.info('Sensors initialized!');
}