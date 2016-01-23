var config = require('../config.js');
var rpi = require('../' + config.rpilib);

//=======================================================
// INITIALIZATION
//=======================================================

var SENSORS_CONFIG = {
	LR_TEMPERATURE: {
		id: 'lr-temp',
		type: 'temperature'
	},
	LR_HUMIDITY: {
		id: 'lr-hum',
		type: 'humidity'
	},
	LR_LUMINOSITY: {
		id: 'lr-lum',
		type: 'luminocity'
	}
};

var SENSOR_IDS = {};
var SENSOR_TYPES = {};
for (var sensorId in SENSORS_CONFIG) {
	SENSOR_IDS[sensorId] = SENSORS_CONFIG[sensorId].id;
	SENSOR_TYPES[SENSORS_CONFIG[sensorId].id] = SENSORS_CONFIG[sensorId].type;
}

var sensors = config.mode == 'debug' ? {} : {
	'DHT11': {
		sensor: new rpi.DHT11({
			pin: 4,
			temperatureId: SENSOR_IDS.LR_TEMPERATURE,
			humidityId: SENSOR_IDS.LR_HUMIDITY
		}),
	},
	'YL-40': {
		sensor: new rpi.YL40Adc({
			inputs: [
			    {
			    	id: SENSOR_IDS.LR_LUMINOSITY,
			    	number: 0
			    }
			]
		}),
		transform: function (vals) {
			vals[SENSOR_IDS.LR_LUMINOSITY] = (255 - vals[SENSOR_IDS.LR_LUMINOSITY]) / 255;
			return vals;
		}
	}
};

var callbacks = {
	onValueReceived: function () {}
};

var values = {};

for (var sensorId in sensors) {
	if (sensors[sensorId].transform == null) {
		sensors[sensorId].transform = function (vals) { return vals; }
	}
}

//=======================================================
// SENSOR FUNCTIONS
//=======================================================

function setValue(sensorId, value) {
	try {
		if (log.trace())
			log.trace('Setting value for sensor "%s" to %d ...', sensorId, value);
		
		values[sensorId] = value;
		callbacks.onValueReceived({
			id: sensorId,
			value: value,
			type: SENSOR_TYPES[sensorId]
		});
	} catch (e) {
		log.error(e, 'Exception while setting value of sensor: %s', sensorId);
	}
}

function readSensors() {
	for (var sensorId in sensors) {
		(function () {
			var config = sensors[sensorId];
			var sensor = config.sensor;
			var transform = config.transform;
			
			sensor.read(function (e, vals) {
				if (e != null) {
					log.error(e, 'Sensor %s had an error while reading!');
					return;
				}
				
				var transformed = transform(vals);
				
				if (log.debug())
					log.debug('Read values: %s', JSON.stringify(transformed));
				
				for (var readingId in transformed) {
					setValue(readingId, transformed[readingId]);
				}
			});
		})();
	}
}

function readAll() {
	if (log.debug())
		log.debug('Reading all devices ...');
	
	readSensors();
}

function mockReadAll() {
	if (log.debug())
		log.debug('Reading all devices ...');
	
	for (var readingId in SENSOR_IDS) {
		var value = Math.random()*100;
		setValue(SENSOR_IDS[readingId], Math.random()*100);
	}
}

//=======================================================
// EXPORTS
//=======================================================

for (var readingId in SENSOR_IDS) {
	exports[readingId] = SENSOR_IDS[readingId];
}

exports.read = config.mode != 'debug' ? readAll : mockReadAll;

exports.getValue = function (sensorId) {
	return sensorId in values ? values[sensorId] : 0;
}

exports.onValueReceived = function (callback) {
	callbacks.onValueReceived = callback;
}

exports.init = function () {
	if (config.samplingInterval == null) throw new Error("The sampling interval is not set!");
	
	if (config.mode != 'debug') {
		log.info('Initializing sensors ...');
		
		for (var sensorId in sensors) {
			sensors[sensorId].sensor.init();
		}
		
		log.info('Starting sampling sensors ...');
		
		readAll();
		setInterval(function () {
			readAll();
		}, config.samplingInterval);
	} else {
		log.info('Initializing mock sensors ...');
		
		mockReadAll();
		setInterval(function () {
			mockReadAll();
		}, config.samplingInterval);
	}
	
	log.info('Sensors initialized!');
}