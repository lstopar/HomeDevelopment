var path = require('path');
var fs = require('fs');

var config = require('../config.js');
var rpi = require('../' + config.rpilib);

//=======================================================
// VARIABLES
//=======================================================

var devices = [];
var sensors = {};
var radio = null;
var values = {};


var callbacks = {
	onValueReceived: function () {}
};

//=======================================================
// INITIALIZATION
//=======================================================

function createDevice(type, devConfig) {
	switch (type) {
	case 'DHT11':
		return new rpi.DHT11(devConfig);
	case 'YL-40':
		return new rpi.YL40Adc(devConfig);
	default:
		throw new Error('Invalid device type: ' + type);
	}
}

function initSensors() {
	log.info('Reading devices configuration ...');
	var devicesConf = require(path.join(__dirname, '../devices', config.devices));
	
	if (log.info())
		log.info('Using device configuration:\n%s', JSON.stringify(devicesConf, null, '\t'));

	for (var deviceN = 0; deviceN < devicesConf.length; deviceN++) {
		log.info('Initializing device %d ...', deviceN);
		var deviceConf = devicesConf[deviceN];
		
		var type = deviceConf.type;
		if (type != 'Rf24') {
			var conf = deviceConf.configuration;
			var deviceSensors = deviceConf.sensors;
			var transform = deviceConf.transform;
			
			for (var sensorN = 0; sensorN < deviceSensors.length; sensorN++) {
				var devSensor = deviceSensors[sensorN];
				
				if (devSensor.id == null) throw new Error('Sensor ID is not defined!');
				if (devSensor.name == null) throw new Error('Sensor name is not defined!');
				
				sensors[devSensor.id] = devSensor;
			}
			
			if (config.mode != 'debug') {
				devices.push({
					device: createDevice(type, conf),
					type: type,
					transform: transform
				});
			}
		} else {
			var nodes = deviceConf.nodes;
			
			var radioSensorH = {};
			var radioConf = [];
			
			for (var nodeN = 0; nodeN < nodes.length; nodeN++) {
				var node = nodes[nodeN];
				var nodeId = node.id;
				
				for (var sensorN = 0; sensorN < node.sensors.length; sensorN++) {
					var sensorConf = node.sensors[sensorN];
					radioSensorH[sensorConf.id] = sensorConf;
					radioConf.push({
						id: sensorConf.id,
						internalId: sensorConf.internalId,
						nodeId: nodeId
					})
				}
			}
			
			radio = {
				sensorH: radioSensorH,
				radio: new rpi.Rf24({
					pinCE: deviceConf.configuration.pinCE,
					pinCSN: deviceConf.configuration.pinCSN,
					sensors: radioConf
				})
			};
		}
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
			type: sensors[sensorId].type
		});
	} catch (e) {
		log.error(e, 'Exception while setting value of sensor: %s', sensorId);
	}
}

function readDevices() {
	for (var deviceN = 0; deviceN < devices.length; deviceN++) {
		(function () {
			var deviceConf = devices[deviceN];
			var device = deviceConf.device;
			var transform = deviceConf.transform;
			
			device.read(function (e, vals) {
				if (e != null) {
					log.error(e, 'Device of type %s had an error while reading!', deviceConf.type);
					return;
				}
				
				var trans = transform != null ? transform(vals) : vals;
				
				if (log.debug())
					log.debug('Read values: %s', JSON.stringify(trans));
				
				for (var sensorId in trans)
					setValue(sensorId, trans[sensorId]);
			});
		})();
	}
}

function readAll() {
	if (log.debug())
		log.debug('Reading all devices ...');
	
	readDevices();
}

function mockReadAll() {
	if (log.debug())
		log.debug('Reading all devices ...');
	
	for (var sensorId in sensors) {
		var value = Math.random()*100;
		setValue(sensorId, value);
	}
}

//=======================================================
// EXPORTS
//=======================================================

//for (var readingId in SENSOR_IDS) {
//	exports[readingId] = SENSOR_IDS[readingId];
//}

exports.getValue = function (sensorId) {
	return sensorId in values ? values[sensorId] : 0;
};

exports.getSensors = function () {
	var result = [];
	for (var sensorId in sensors) {
		result.push(sensors[sensorId]);
	}
	return result;
};

exports.init = function () {
	if (config.samplingInterval == null) throw new Error("The sampling interval is not set!");
	
	log.info('Initializing Rpi');
	rpi.init();
	initSensors();
	
	if (config.mode != 'debug') {
		log.info('Initializing devices ...');
		
		for (var deviceN = 0; deviceN < devices.length; deviceN++) {
			devices[deviceN].device.init();
		}
		
		log.info('Starting sampling sensors ...');
		readAll();
		setInterval(function () {
			try {
				readAll();
			} catch (e) {
				log.error(e, 'Exception while reading all devices!');
			}
		}, config.samplingInterval);
	} else {
		mockReadAll();
		setInterval(function () {
			mockReadAll();
		}, config.samplingInterval);
	}
	
	log.info('Sensors initialized!');
}

exports.onValueReceived = function (callback) {
	callbacks.onValueReceived = callback;
}