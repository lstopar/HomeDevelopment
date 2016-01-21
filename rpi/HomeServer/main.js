var config = require('./config.js');
var rpi = require(config.rpilib);

function hackClasses() {
	if (!String.prototype.startsWith) {
		String.prototype.startsWith = function(searchString, position) {
			position = position || 0;
	    	return this.indexOf(searchString, position) === position;
	  	};
	}
	if (!String.prototype.endsWith) {
		String.prototype.endsWith = function(searchString, position) {
			var subjectString = this.toString();
			if (position === undefined || position > subjectString.length) {
				position = subjectString.length;
			}
			position -= searchString.length;
			var lastIndex = subjectString.indexOf(searchString, position);
			return lastIndex !== -1 && lastIndex === position;
		};
	}
}

try {
	hackClasses();
	
	var sensor = new rpi.DHT11(4);
	sensor.init();
	
	sensor.read(function (e, result, result1) {
		if (e != null) {
			log.error(e, 'Failed to read sensor!');
			return;
		}
		
		log.info('result: %s', JSON.stringify(result));
		log.info('result1: %s', JSON.stringify(result1));
	});
	
	var resultSync = sensor.readSync();
	log.info('resultSync: %s', JSON.stringify(resultSync));
} catch (e) {
	log.error(e, 'Exception in main, exiting ...');
	process.exit(1);
}