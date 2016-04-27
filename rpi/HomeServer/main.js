var SegfaultHandler = require('segfault-handler');

var config = require('./config.js');
var server = require('./src/server.js');
var sensors = require('./src/sensors.js');

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
	SegfaultHandler.registerHandler("crash.log");
	
	hackClasses();
	
	sensors.init();
	server.init({
		sensors: sensors
	});
} catch (e) {
	log.error(e, 'Exception in main, exiting ...');
	process.exit(1);
}