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
var values = {};
var layoutGroups = [];


var callbacks = {
	onValueReceived: function () {},
	onNodeEvent: function () {}
};

//=======================================================
// SENSOR FUNCTIONS
//=======================================================

function setValue(sensorId, value) {
	if (sensorId in sensors) {
		var device = sensors[sensorId].device;
		device.set({ id: sensorId, value: value });
	} else if (sensorId in radio.sensorH) {
		var success = radio.radio.set({ id: sensorId, value: value });
		onNodeConnected(radio.sensorH[sensorId].nodeId, success);
	} else {
		throw new Error('Could not find sensor: ' + sensorId);
	}
}

function updateValue(sensorId, value) {
	try {
		if (log.trace())
			log.trace('Setting value for sensor "%s" to %d ...', sensorId, value);
		
		var type = sensorId in sensors ? sensors[sensorId] : radio.sensorH[sensorId].type;
		
		values[sensorId] = value;
		callbacks.onValueReceived({
			id: sensorId,
			value: value,
			type: type
		});
		
		if (devices.onValue != null) {
			devices.onValue(sensorId, value);
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
				
				if (log.debug())
					log.debug('Read values: %s', JSON.stringify(trans));
				
				for (var sensorId in trans)
					updateValue(sensorId, trans[sensorId]);
			});
		})();
	}
}

function pingRadios() {
	if (log.debug())
		log.debug('Pinging radio devices ...');
	
	try {
		var nodeIdH = radio.nodes;
		
		for (var nodeId in nodeIdH) {			
			if (log.debug())
				log.debug('Pinging node %d', nodeId)
			
			onNodeConnected(nodeId, radio.radio.ping(parseInt(nodeId)));
		}
	} catch (e) {
		log.error(e, 'Exception while pinging radio nodes!');
	}
}

function readRadioDevices() {
	if (radio != null) {
		var radioSensorH = radio.sensorH;
		for (var id in radioSensorH) {
			var nodeId = radioSensorH[id].nodeId;
			
			if (log.debug())
				log.debug('Calling get on radio ...');
			
			var success = radio.radio.get(id);
			onNodeConnected(nodeId, success);
		}
	}
}

function readAll() {
	if (log.debug())
		log.debug('Reading all devices ...');
	
	readDevices();
	readRadioDevices();
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
			
			var sensorConf = sensorId in sensors ? sensors[sensorId] : radio.sensorH[sensorId];
			
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
			layoutGroups.push({
				id: 'group-' + sensorId,
				img: utils.getSensorImg(sensors[sensorId]),
				sensors: [sensors[sensorId]]
			});
		}
	}
	
	log.info('Layout initialized!');
}

function initSensors() {
	log.info('Reading devices configuration ...');
	var devs = require(path.join(__dirname, '../devices', config.devices))(setValue)
	var devicesConf = devs.devices;
	var onValueCb = devs.onValue != null ? devs.onValue : function () {};
	
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
				
				if (devSensor.id == null) throw new Error('Sensor ID is not defined!');
				if (devSensor.name == null) throw new Error('Sensor name is not defined!');
				
				devSensor.device = device;
				
				sensors[devSensor.id] = devSensor;
			}
		} else {
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
					
					// add the node ID, so it can be quickly accessed later
					sensorConf.nodeId = nodeId;
					
					radioSensorH[sensorConf.id] = sensorConf;
					radioConf.push({
						id: sensorConf.id,
						internalId: sensorConf.internalId,
						nodeId: nodeId
					})
				}
				
				nodeIdH[nodeId] = {
					id: nodeId,
					name: node.name,
					connected: false
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
			radio.radio.onValue(function (val) {	// TODO move this somewhere, make a common interface
				if (log.debug()) 
					log.debug('Received value from the radio: %s', JSON.stringify(val));
				updateValue(val.id, val.value);
				onValueCb(val.id, val.value);
			});
		}
	}
	
	initGroups(devs.layout);
}

//=======================================================
// EXPORTS
//=======================================================

exports.getValue = function (sensorId) {
	return sensorId in values ? values[sensorId] : 0;
};

exports.setValue = setValue;

exports.getSensors = function () {
	var result = [];
	if (radio != null) {
		for (var sensorId in radio.sensorH) {
			result.push(radio.sensorH[sensorId]);
		}
	}
	for (var sensorId in sensors) {
		result.push(sensors[sensorId]);
	}
	return result;
};

exports.getLayout = function () {
	if (log.trace()) {
		log.debug('Returning layout: %s', JSON.stringify(layoutGroups));
	}
	return layoutGroups;
}

exports.isOnline = function (nodeId) {
	if (radio == null) return false;
	return radio.nodes[nodeId].connected;
}

exports.getNodes = function () {
	if (radio == null) return [];
	
	var result = [];
	for (var nodeId in radio.nodes) {
		result.push(radio.nodes[nodeId]);
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
		
		if (radio != null) {
			log.info('Initializing radio ...');
			radio.radio.init();
			setInterval(pingRadios, config.pingInterval);
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

exports.onNodeEvent = function (callback) {
	callbacks.onNodeEvent = callback;
}