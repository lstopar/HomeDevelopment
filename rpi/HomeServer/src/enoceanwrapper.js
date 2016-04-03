var config = require('../config.js');
var enocean = require(config.enoceanLib);

var gateway = null;

var callbacks = {
	onValue: function (sensorId, value) {},
	onDeviceLearned: function (id, type) {}
};

var devices = function () {
	var deviceH = {};
	var internalToExternalIdH = {};
	var sensorToDeviceIdH = {};
	
	var that = {
		add: function (device) {
			var deviceId = device.id;
			
			deviceH[deviceId] = {
				config: {
					internalId: device.internalId,
				},
				device: undefined,
				sensors: {},
				internalToExternalSensorIdH: {}
			};
			internalToExternalIdH[device.internalId] = deviceId;
			
			var sensors = deviceH[deviceId].sensors;
			for (var sensorN = 0; sensorN < device.sensors.length; sensorN++) {
				var sensor = device.sensors[sensorN];
				var sensorId = sensor.id;
				
				sensors[sensorId] = sensor;
				sensorToDeviceIdH[sensorId] = deviceId;
				deviceH[deviceId].internalToExternalSensorIdH[sensor.internalId] = sensorId;
			}
		},
		
		hasDevice: function (internalId) {
			return internalId in internalToExternalIdH;
		},
		
		hasSensor: function (sensorId) {
			return sensorId in sensorToDeviceIdH;
		},
		
		initDevice: function (device) {
			var internalId = device.id;
			
			if (!that.hasDevice(internalId)) throw new Error('Do not have external ID for device: ' + internalId);
			
			var deviceId = internalToExternalIdH[internalId];
			deviceH[deviceId].device = device;
		},
		
		readAll: function () {
			for (var deviceId in deviceH) {
				var device = deviceH[deviceId].device;
				
				if (device == null) {
					log.warn('Device %s not initialized!', deviceId);
					continue;
				}
				
				switch (device.type) {
				case 'D2-01-xx':
					device.readStatus();
					break;
				default:
					throw new Error('Unknown device type: ' + device.type);
				}
			}
		},
		
		set: function (sensorId, value) {
			if (!(sensorId in sensorToDeviceIdH)) throw new Error('Tried to set a value for an invalid sensorId: ' + sensorId);
			
			var deviceId = sensorToDeviceIdH[sensorId];
			var conf = deviceH[deviceId];
			var device = conf.device;
			
			if (device == null) throw new Error('Could not find device for: %s', sensorId);
			
			switch (device.type) {
			case 'D2-01-xx':
				var sensorConf = conf.sensors[sensorId];
				
				if (sensorConf == null) throw new Error('Could not find sensor configuration for sensor: ' + sensorId);
				
				var channelN = sensorConf.internalId;
				
				device.setOutput(channelN, value);
				break;
			default:
				throw new Error('Unknown device type: ' + device.type);
			}
		},
		
		getExternalDeviceId: function (internalId) {
			if (!(internalId in internalToExternalIdH)) throw new Error('Internal device ID: ' + internalId + ' missing!');
			return internalToExternalIdH[internalId];
		},
		
		getInternalDeviceId: function (externalId) {
			if (!externalId in deviceH) throw new Error('Device ' + externalId + ' missing!');
			return deviceH[externalId].config.internalId;
		},
		
		getInternalSensorId: function (externalId) {
			if (!(externalId in sensorToDeviceIdH)) throw new Error('Device for sensor ID: ' + externalId + ' missing!');
			
			var deviceId = sensorToDeviceIdH[externalId];
			var device = deviceH[deviceId];
			var sensors = device.sensors;
			var sensor = sensors[externalId];
			return sensor.internalId;
		},
		
		getExternalSensorId: function (internalDeviceId, internalId) {
			if (!(internalDeviceId in internalToExternalIdH)) throw new Error('Device ID: ' + deviceId + ' not found!');
			
			var deviceId = internalToExternalIdH[internalDeviceId];
			var dev = deviceH[deviceId];
			
			return dev.internalToExternalSensorIdH[internalId];
		},
		
		getAllSensorIds: function () {
			var result = [];
			
			for (var deviceId in deviceH) {
				var conf = deviceH[deviceId];
				for (var sensorId in conf.sensors) {
					result.push(sensorId);
				}
			}
			
			return result;
		},
		
		getSensor: function (sensorId) {
			if (!that.hasSensor(sensorId)) throw new Error('Unknown device for sensor: ' + sensorId);
			
			var deviceId = sensorToDeviceIdH[sensorId];
			var device = deviceH[deviceId];
			
			return device.sensors[sensorId];
			return devices.getSensor(sensorId);
		}
	}
	
	return that;
}();


function initHandlers(device) {
	switch (device.type) {
	case 'D2-01-xx':
		device.on('status', function (channel, value) {
			var sensorId = devices.getExternalSensorId(device.id, channel);
			callbacks.onValue(sensorId, value);
		});
		break;
	default: {
		throw new Error('Unknown EnOcean device type: ' + device.type);
	}
	}
}

module.exports = exports = function (opts) {
	var config = opts.configuration;
	
	for (var nodeN = 0; nodeN < opts.nodes.length; nodeN++) {
		var device = opts.nodes[nodeN];
		devices.add(device);
	}
	
	log.info('Creating Gateway ...');
	gateway = new enocean.Gateway(config);
	gateway.on('device', function (device) {
		log.info('New EnOcean device of type %s', device.type);
		
		var internalId = device.id;
		
		try {
			if (devices.hasDevice(internalId)) {
				var externalId = devices.getExternalDeviceId(internalId);
				log.info('initializing EnOcean device with ID: %s ...', externalId);
				
				initHandlers(device);
				devices.initDevice(device);
			} else {
				log.info('New EnOcean device of type %s: %d', device.type, device.id);
				callbacks.onDeviceLearned(device.id, device.type);
			}
		} catch (e) {
			log.error(e, 'Failed to initialize device: %d', internalId);
		}
	});
	
	return {
		init: function () {
			log.info('Inializing EnOcean ...');
			gateway.init();
		},
		startLearningMode: function () {
			log.info('Starting learning mode ...');
			gateway.startLearningMode();
		},
		readAll: function () {
			devices.readAll();
		},
		
		set: function (sensorId, value) {
			devices.set(sensorId, value);
		},
		
		on: function (eventId, callback) {
			switch (eventId) {
			case 'value':
				callbacks.onValue = callback;
				break;
			case 'deviceLearned':
				callbacks.onDeviceLearned = callback;
				break;
			default:
				throw new Error('Unknown event id: ' + eventId);
			}
		},
		
		getSensors: function () {
			return devices.getAllSensorIds();
		},
		
		getSensor: function (sensorId) {
			return devices.getSensor(sensorId);
		},
		
		hasSensor: function (sensorId) {
			return devices.hasSensor(sensorId);
		}
	}
};