var enocean = require('./build/Debug/enoceannode.node');

var gateway = new enocean.Gateway({
	serialPort: '/dev/ttyUSB0',
	storageFile: 'gateway-devices.txt',
	verbose: true
});

gateway.onDeviceConnected(function (device) {
	console.log('Got device in JS: ' + device.getId());
});

gateway.init();
gateway.startLearningMode();