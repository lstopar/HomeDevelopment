var config = require('../config.js');
var rpi = require(config.rpilib);

var sensors = {
	'DHT11': {
		sensor: new rpi.DHT11(4),
		onRead: function (e, vals) {
			if (e != null) {
				log.error(e, 'Failed to read sensor!');
				return;
			}
			
			log.info('result: %s', JSON.stringify(result));
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
			
			log.info('Got output from ADC: %s', JSON.stringify(result));
		}
	}
};
var values = {};

for (var sensorId in sensors) {
	if (sensors[sensorId].onRead == null) throw new Error('Sensor ' + sensorId + ' has no callback function!');
}

//function initSensorPool() {
//	
//}
exports.read = function () {
	for (var sensorId in sensors) {
		var config = sensors[sensorId];
		var sensor = config.sensor;
		var callback = config.onRead;
		
		sensor.read(callback);
	}
}

module.init = function () {
	log.info('Initializing sensors ...');
	
	for (var sensorId in sensors) {
		sensors[sensorId].init();
	}
	
	log.info('Sensors initialized!');
}