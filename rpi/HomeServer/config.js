var bunyan = require('bunyan');
var logformat = require('bunyan-format');
var fs = require('fs');

//read the configuration file
var confFile = process.argv[2];

console.log('Reading configuration file: ' + confFile);
var configStr = fs.readFileSync(confFile);
var config = JSON.parse(configStr);

//================================================================
//LOG
//================================================================

var loggerConfig = config.log.logger;
var stream;
if (loggerConfig.stream.type == 'stdout') {
	console.log('Using stdout as log stream ...');
	stream = process.stdout;
} else {	// TODO file stream doesn't work
	console.log('Using file \'' + loggerConfig.stream.file + '\' as log stream ...');
	stream = fs.createWriteStream(loggerConfig.stream.file);
}
var logStream = {
	outputMode: loggerConfig.outputMode,
	out: stream
};

global.log = bunyan.createLogger({
	name: 'FFA',
	stream: logformat(logStream),
	level: config.log.logger.level
});

//================================================================
// CONFIGURE SESSION IDLE TIME
//================================================================
var idleTimeStr = config.session.maxIdleTime;

var val = parseInt(idleTimeStr.substring(0, idleTimeStr.length-1));
var unit = idleTimeStr.substring(idleTimeStr.length-1);

if (isNaN(val)) {
	log.error(new Error('Invalid value: ' + val), 'Exception while parsing input number!');
	process.exit(1);
}

switch (unit) {
case 's':
	val *= 1000;
	break;
case 'm':
	val *= 1000*60;
	break;
case 'h':
	val *= 1000*60*60;
	break;
case 'd':
	val *= 1000*60*60*24;
	break;
default:
	log.error(new Error('Unknown unit: ' + unit), 'Exception while parsing input number!');
	process.exit(2);
}

config.session.maxIdleTime = val;


//================================================================
//EXPORTS
//================================================================

module.exports = config;

//================================================================
//PRINT
//================================================================

log.info('Using configuration:\n%s', configStr);