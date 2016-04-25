var http = require('http');

var config = require('../config.js');

var MAX_CACHE_SIZE = 1000000;
var batchSize = config.logger.batchSize != null ? config.logger.batchSize : 1000;

var values = [];
var requestActive = false;


exports.log = function (value) {
	try {
		var transformed = {
			timestamp: new Date().getTime()
		};
		for (var key in value) {
			transformed[key] = value[key];
		}
		
		values.push(transformed);
		
		if (values.length > batchSize && !requestActive) {
			log.info('Sending batch, values in queue: %d ...', values.length);
			var batch = values.splice(0, batchSize);
			var data = JSON.stringify(batch);
			
			var options = {
				host: config.logger.host,
				port: config.logger.port,
				path: config.logger.path,
				method: 'POST',
				headers: {
					'Content-Type': 'application/json',
					'Content-Length': Buffer.byteLength(data)
				}
			};
			
			requestActive = true;
			var req = http.request(options, function (res) {
				res.on('data', function (chunk) {
					if (log.debug())
						log.debug('Received chunk from logging server: %s', chunk);
				});
				res.on('end', function () {
					if (res.statusCode >= 300) {
						values = batch.concat(values);
						log.warn('Failed to send values to sever!');
					} else {
						log.info('Successfully posted the batch, items in queue: %d!', values.length);
					}
					
					requestActive = false;
				});
			});
			
			req.on('error', function (e) {
				log.error(e, 'Exception while posting to server!');
				values = batch.concat(values);
				requestActive = false;
			});
			
			req.write(data);
			req.end();
		}
		
		if (values.length > MAX_CACHE_SIZE) {
			log.warn('The maximum cache size exceeded, must remove values!');
			values.splice(0, batchSize);
			log.info('Cleared %d values from the cache, items remaining: %d!', batchSize, values.length);
		}
	} catch (e) {
		log.error(e, 'Exception while posting data to server!');
	}
};