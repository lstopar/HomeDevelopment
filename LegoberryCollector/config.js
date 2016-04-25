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
//EXPORTS
//================================================================

module.exports = config;

//================================================================
//PRINT
//================================================================

log.info('Using configuration:\n%s', configStr);