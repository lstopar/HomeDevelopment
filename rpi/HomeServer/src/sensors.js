var path = require('path');
var fs = require('fs');

var config = require('../config.js');
var utils = require('./utils.js');
var rpi = require('../' + config.rpilib);

//=======================================================
// VARIABLES
//=======================================================

var devices = [];
var sensors = {};
var radio = null;
var enocean = null;
var values = {};
var layoutGroups = [];

var onValueCallback = function () {};


var callbacks = {
	onValueReceived: function () {},
	onNodeEvent: function () {}
};

//=======================================================
// SENSOR FUNCTIONS
//=======================================================

function getSensorConfig(sensorId) {
	if (sensorId in sensors) {
		return sensors[sensorId];
	} else if (sensorId in radio.sensorH) {
		return radio.sensorH[sensorId];
	} else if (enocean != null && enocean.hasSensor(sensorId)) {
		return enocean.getSensor(sensorId);
	} else {
		throw new Error('Could not find the type for sensor: ' + sensorId);
	}
}

function getValue(sensorId) {
	return sensorId in values ? values[sensorId] : 0;
}

function setValue(vals) {
	if (vals.constructor === Array) {
		var rpiDevices = [];
		var radioDevices = [];
		var enoceanDevices = [];
		
		for (var i = 0; i < vals.length; i++) {
			var sensorId = vals[i].sensorId;
			
			if (sensorId in sensors)
				rpiDevices.push(vals[i]);
			else if (sensorId in radio.sensorH)
				radioDevices.push(vals[i]);
			else if (enocean != null && enocean.hasSensor(sensorId))
				enoceanDevices.push(vals[i]);
			else
				throw new Error('Could not find sensor: ' + sensorId);
		}
		
		// set RPi devices
		for (var i = 0; i < rpiDevices.length; i++) {
			setValue(rpiDevices[i]);
		}
		// set radio devices
		if (radioDevices.length > 0) {
			if (log.debug())
				log.debug('Setting %d radio values ...', radioDevices.length);
			
			radio.radio.set(radioDevices);
		}
		// set EnOcean devices
		for (var i = 0; i < enoceanDevices.length; i++) {
			setValue(enoceanDevices[i]);
		}
	} else {
		var sensorId = vals.sensorId;
		var value = vals.value;
		
		if (sensorId in sensors) {
			var device = sensors[sensorId].device;
			device.set({ id: sensorId, value: value });
		}
		else if (sensorId in radio.sensorH) {
			var success = radio.radio.set({ sensorId: sensorId, value: value });
			onNodeConnected(radio.sensorH[sensorId].nodeId, success);
		}
		else if (enocean.hasSensor(sensorId)) {
			enocean.set(sensorId, value);
		} 
		else {
			throw new Error('Could not find sensor: ' + sensorId);
		}
	}
}

function updateValue(sensorId, value) {
	try {
		if (log.trace())
			log.trace('Updating value for sensor "%s" to %d ...', sensorId, value);
		
		var previousVal = values[sensorId]; 
		
		if (value != previousVal) {
			values[sensorId] = value;
			
			if (log.debug())
				log.debug('Value of %s changed to %s', sensorId, value);
			
			var type = getSensorConfig(sensorId).type;
			
			// web sockets callback
			callbacks.onValueReceived({
				id: sensorId,
				value: value,
				type: type
			});
			// devices callback
			onValueCallback(sensorId, value);
		}
	} catch (e) {
		log.error(e, 'Exception while setting value of sensor: %s', sensorId);
	}
}

function onNodeConnected(nodeId, connected) {
	try {
		var nodeIdH = radio.nodes;
		var prevStatus = nodeIdH[nodeId].connected;
		
		if (connected != prevStatus) {
			nodeIdH[nodeId].connected = connected;
			nodeIdH[nodeId].connectionChangedTime = new Date().getTime();
			
			if (connected) {
				log.info('Node %d has connected!', nodeId);
			} else {
				log.warn('Node %d has disconnected!', nodeId);
			}
			
			callbacks.onNodeEvent({
				id: nodeId,
				name: nodeIdH[nodeId].name,
				connected: connected
			});
		}
	} catch (e) {
		log.error(e, 'Exception while processing node connected event!');
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
				
				if (log.trace())
					log.trace('Read values: %s, updating ...', JSON.stringify(trans));
				
				for (var sensorId in trans)
					updateValue(sensorId, trans[sensorId]);
			});
		})();
	}
}

var rf24Pinger = (function () {
	var nodePongH = {};
	
	return {
		init: function () {	// TODO is not used
			for (var nodeId in radio.nodes) {
				nodePongH[nodeId] = true;
			}
		},
		onPong: function (nodeId) {	// TODO is not used
//			log.info('Node %d ponged!', nodeId);
			nodePongH[nodeId] = true;
		},
		ping: function () {
			if (log.trace())
				log.trace('Pinging radio devices ...');
			
			try {
				var nodeIdH = radio.nodes;
				
				for (var nodeId in nodeIdH) {
					if (log.trace())
						log.trace('Pinging node %d', nodeId)
					
					var success = radio.radio.ping(parseInt(nodeId));
					onNodeConnected(nodeId, success);
				}
			} catch (e) {
				log.error(e, 'Exception while pinging radio nodes!');
			}
		}
	}
})();

function readRadioDevices() {
	if (radio != null) {
		if (log.trace())
			log.trace('Fetching all values from radio devices ...');
		
		var nodes = radio.nodes;
		for (var nodeId in nodes) {
			if (log.trace())
				log.trace('Fetching all values from node %d', nodeId);
			var success = radio.radio.getAll(parseInt(nodeId));
			onNodeConnected(nodeId, success);
		}
	}
}

function readEnocean() {
	if (enocean == null) return;
	
	try {
		if (log.trace())
			log.trace('Fetching all values from EnOcean devices ...');
		
		enocean.readAll();
	} catch (e) {
		log.error(e, 'Failed to read all EnOcean devices!');
	}
}

function readAll() {
	if (log.trace())
		log.trace('Reading all devices ...');
	
	readDevices();
	readRadioDevices();
	readEnocean();
}

function mockReadAll() {
	if (log.debug())
		log.debug('Reading all devices ...');
	
	for (var sensorId in sensors) {
		var value = Math.random()*100;
		updateValue(sensorId, value);
	}
}

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

function initGroups(layout) {
	log.info('Constructing layout groups:\n%s', JSON.stringify(layout, null, '\t'));
	
	if (layout == null) { layout = []; }
	
	var usedSensorIds = {};
	
	for (var groupN = 0; groupN < layout.length; groupN++) {
		var group = layout[groupN];
		var sensorIds = group.sensorIds;
		
		var layoutGroup = {
			id: group.id,
			img: group.img,
			sensors: []
		};
		
		for (var sensorN = 0; sensorN < sensorIds.length; sensorN++) {
			var sensorId = sensorIds[sensorN];
			
			if (sensorId in usedSensorIds) {
				throw new Error('Sensor ID already present in the layout!');
			}
			
			var sensorConf = getSensorConfig(sensorId);
			if (sensorConf == null) {
				throw new Error('Failed to get configuration of sensor: ' + sensorId);
			}
			
			usedSensorIds[sensorId] = true;
			layoutGroup.sensors.push(sensorConf);
		}
		
		layoutGroups.push(layoutGroup);
	}
	
	for (var sensorId in sensors) {
		if (!(sensorId in usedSensorIds)) {
			var sensor = sensors[sensorId];
			
			if (sensor.hidden) continue;
			
			layoutGroups.push({
				id: 'group-' + sensorId,
				img: utils.getSensorImg(sensor),
				sensors: [sensor]
			});
		}
	}
	
	for (var sensorId in radio.sensorH) {
		if (!(sensorId in usedSensorIds)) {
			var sensor = radio.sensorH[sensorId];
			
			if (sensor.hidden) continue;
			
			layoutGroups.push({
				id: 'group-' + sensorId,
				img: utils.getSensorImg(sensor),
				sensors: [sensor]
			});
		}
	}
	
	if (enocean != null) {
		var eoSensors = enocean.getSensors();
		for (var i = 0; i < eoSensors.length; i++) {
			var sensorId = eoSensors[i];
			
			if (sensorId in usedSensorIds) continue;
			
			var sensor = enocean.getSensor(sensorId);
			
			if (sensor.hidden) continue;
			
			layoutGroups.push({
				id: 'group-' + sensorId,
				img: utils.getSensorImg(sensor),
				sensors: [sensor]
			});
		}
	}
	
	log.info('Layout initialized!');
}

function initSensors() {
	log.info('Reading devices configuration ...');
	var devs = require(path.join(__dirname, '../devices', config.devices))(getValue, setValue)
	var devicesConf = devs.devices;
	
	onValueCallback = devs.onValue != null ? devs.onValue : function () {};
	var transformH = {};
	
	if (log.info())
		log.info('Using device configuration:\n%s', JSON.stringify(devicesConf, null, '\t'));

	for (var deviceN = 0; deviceN < devicesConf.length; deviceN++) {
		log.info('Initializing device %d ...', deviceN);
		var deviceConf = devicesConf[deviceN];
		
		var type = deviceConf.type;
		if (type == 'Rf24') {
			log.info('Initializing radio ...');
			
			var nodes = deviceConf.nodes;
			
			var radioSensorH = {};
			var radioConf = [];
			var nodeIdH = {};
			
			for (var nodeN = 0; nodeN < nodes.length; nodeN++) {
				var node = nodes[nodeN];
				var nodeId = node.id;
				
				for (var sensorN = 0; sensorN < node.sensors.length; sensorN++) {
					var sensorConf = node.sensors[sensorN];
					var id = sensorConf.id;
					
					log.info('Initializing sensor %s ...', id);
					
					// add the node ID, so it can be quickly accessed later
					sensorConf.nodeId = nodeId;
					
					radioSensorH[id] = sensorConf;
					radioConf.push({
						id: id,
						internalId: sensorConf.internalId,
						nodeId: nodeId
					});
					transformH[sensorConf.id] = sensorConf.transform != null ? 
							sensorConf.transform : function (val) { return val; }
				}
				
				nodeIdH[nodeId] = {
					id: nodeId,
					name: node.name,
					connected: false,
					connectionChangedTime: new Date().getTime()
				};
			}
			
			var conf = deviceConf.configuration;
			conf.sensors = radioConf;
			
			log.info('Creating ...');
			radio = {
				sensorH: radioSensorH,
				nodes: nodeIdH,
				radio: new rpi.Rf24(conf)
			};
			
			log.info('Setting callback ...');
			radio.radio.on('value', function (val) {	// TODO move this somewhere, make a common interface
				var transFun = transformH[val.id];
				var trans = transFun(val.value);
				
				if (log.debug())
					log.debug('Received from radio %s: %d', val.id, trans);
				
				updateValue(val.id, trans);
			});
			radio.radio.on('pong', function (nodeId) {
				log.info('Received pong from node %d, ignoring ...', nodeId);
//				rf24Pinger.onPong(nodeId + '');
			});
		}
		else if (type == 'EnOcean') {
			log.info('Initializing EnOcean gateway ...');
			
			enocean = require('./enoceanwrapper.js')(deviceConf);
			
			var enoceanSensors = enocean.getSensors();
			for (var i = 0; i < enoceanSensors.length; i++) {
				var sensorId = enoceanSensors[i];
				var sensorConf = enocean.getSensor(sensorId);
				transformH[sensorId] = sensorConf.transform != null ? sensorConf.transform : function (val) { return val; };
			}
			
			log.info('Adding value listener ...');
			enocean.on('value', function (sensorId, value) {
				var transFun = transformH[sensorId];
				var trans = transFun(value);
				
				if (log.debug())
					log.debug('Received from enocean %s: %d', sensorId, trans);
				
				updateValue(sensorId, trans);
			});
			
			log.info('Adding device listener ...');
			enocean.on('deviceLearned', function (deviceId, type) {
				log.info('Learned new EnOcean device of type %s: %d', type, deviceId);
				// TODO
			});
		}
		else if (type == 'virtual') {
			var deviceSensors = deviceConf.sensors;
			
			for (var sensorN = 0; sensorN < deviceSensors.length; sensorN++) {
				(function () {
					var devSensor = deviceSensors[sensorN];
					var id = devSensor.id;
					var controller = devSensor.controller;
					
					if (id == null) throw new Error('Sensor ID is not defined!');
					if (devSensor.name == null) throw new Error('Sensor name is not defined!');
					if (controller == null) throw new Error('Controller not defined for virtual sensor %s', devSensor.name);
					if (controller.read == null) throw new Error('Read is not defined for virtual sensor %s!', devSensor.name);
					if (controller.setOnChange == null) throw new Error('setOnChange is not defined for virtual sensor %s!', devSensor.name);
					
					log.info('Initializing sensor %s ...', id);
					
					controller.setOnChange(function (val) {
						if (log.debug())
							log.debug('Received virtual sensor value %s', JSON.stringify(val));
						updateValue(val.id, val.value);
					});
					
					var device = {
						read: controller.read,
						init: controller.init != null ? controller.init : function () {}
					};
					
					devices.push({
						device: device,
						type: type
					});
					devSensor.device = device;
					
					sensors[id] = devSensor;
				})();
			}
		} else {	// normal device
			var conf = deviceConf.configuration;
			var deviceSensors = deviceConf.sensors;
			var transform = deviceConf.transform;
			
			var device = null;
			
			if (config.mode != 'debug') {
				device = createDevice(type, conf);
				
				devices.push({
					device: device,
					type: type,
					transform: transform
				});
			}
			
			for (var sensorN = 0; sensorN < deviceSensors.length; sensorN++) {
				var devSensor = deviceSensors[sensorN];
				var id = devSensor.id;
				
				if (id == null) throw new Error('Sensor ID is not defined!');
				if (devSensor.name == null) throw new Error('Sensor name is not defined!');
				
				log.info('Initializing sensor %s ...', id);
				
				devSensor.device = device;
				
				sensors[devSensor.id] = devSensor;
			}
		}
	}
	
	initGroups(devs.layout);
}

//=======================================================
// EXPORTS
//=======================================================

exports.getValue = getValue;
exports.setValue = setValue;

exports.getSensors = function () {
	var result = [];
	if (radio != null) {
		for (var sensorId in radio.sensorH) {
			result.push(getSensorConfig(sensorId));
		}
	}
	for (var sensorId in sensors) {
		result.push(getSensorConfig(sensorId));
	}
	if (enocean != null) {
		var enoceanSensors = enocean.getSensors();
		for (var i = 0; i < enoceanSensors.length; i++) {
			result.push(getSensorConfig(enoceanSensors[i]));
		}
	}
	return result;
};

exports.getLayout = function () {
	if (log.trace())
		log.trace('Returning layout: %s', JSON.stringify(layoutGroups));
	return layoutGroups;
};

exports.isOnline = function (nodeId) {
	if (radio == null) return false;
	return radio.nodes[nodeId].connected;
};

exports.getConnChangedTime = function (nodeId) {
	if (radio == null || !(nodeId in radio.nodes)) return Number.MAX_VALUE;
	return radio.nodes[nodeId].connectionChangedTime;
}

exports.getNodes = function () {
	if (radio == null) return [];
	
	var result = [];
	for (var nodeId in radio.nodes) {
		result.push(radio.nodes[nodeId]);
	}
	
	return result;
};

exports.hasEnOcean = function () {
	return enocean != null;
};

exports.enterEoLearnMode = function () {
	if (!exports.hasEnOcean()) throw new Error('Tried to enter learn mode with no EnOcean device!');
	
	enocean.startLearningMode();
}

exports.init = function () {
	if (config.samplingInterval == null) throw new Error("The sampling interval is not set!");
	
	log.info('Initializing Rpi');
	rpi.init();
	initSensors();
	
	log.info('Initializing devices ...');
	
	for (var deviceN = 0; deviceN < devices.length; deviceN++) {
		devices[deviceN].device.init();
	}
	
	if (radio != null) {
		log.info('Initializing radio ...');
		radio.radio.init();
		rf24Pinger.init();
		setInterval(rf24Pinger.ping, config.pingInterval);
	}
	
	if (enocean != null) {
		log.info('Initialzing EnOcean radio ...');
		enocean.init();
	}
	
	log.info('Starting sampling sensors ...');
	setTimeout(function () {
		setInterval(function () {
			try {
				readAll();
			} catch (e) {
				log.error(e, 'Exception while reading all devices!');
			}
		}, config.samplingInterval);
	}, 5000);
	
	log.info('Sensors initialized!');
}

exports.onValueReceived = function (callback) {
	callbacks.onValueReceived = callback;
}

exports.onNodeEvent = function (callback) {
	callbacks.onNodeEvent = callback;
}