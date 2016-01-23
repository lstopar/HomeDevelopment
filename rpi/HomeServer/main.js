var config = require('./config.js');
var sensors = require('./src/sensors.js');
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
	
	sensors.init();
} catch (e) {
	log.error(e, 'Exception in main, exiting ...');
	process.exit(1);
}