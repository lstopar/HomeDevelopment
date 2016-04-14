var mysql = require('mysql');

var config = require('./config.js');

log.info('Creating MySQL connection pool ...');

var db = (function () {
	var pool = mysql.createPool({
		database: config.database.database,
		host: config.database.host,
		user: config.database.user,
		password: config.database.password,
		connectionLimit: 10,
		multipleStatements: true
	});

	function constructMultipleInsert(table, keys, fields) {
		if (fields == null || fields.length == 0) return null;
		
		var formatStr = '(';
		for (var i = 0; i < keys.length; i++) {
			formatStr += '?';
			if (i < keys.length - 1)
				formatStr += ',';
		}
		formatStr += ')';
		
		var sql = 'INSERT INTO ' + table + ' ' + mysql.format(formatStr, keys).replace(/'/g, '') + ' VALUES ';
		
		for (var i = 0; i < fields.length; i++) {
			var insert = [];
			for (var keyN = 0; keyN < keys.length; keyN++) {
				insert.push(fields[i][keys[keyN]]);
			}
			sql += mysql.format(formatStr, insert);
			if (i < fields.length-1)
				sql += ',';
		}
		
		return sql;
	}
	
	return {
		insertReadings: function (readings) {
			if (log.debug())
				log.debug('Inserting readings ...');
			
			var sql = constructMultipleInsert('readings', ['sensorId', 'value', 'timestamp'], readings);
			
			pool.getConnection(function (e, conn) {
				if (e != null) {
					log.error(e, 'Failed to create a database connection!');
					if (conn != null) conn.release();
					return;
				}
				
				try {
					var q = conn.query(sql, [], function (e1) {
						if (e1 != null) {
							log.error('Exception while executing insert statement!');
							return;
						}
						
						if (log.debug())
							log.debug('Inserted!');
						
						if (conn != null)
							conn.release();
					});
				} catch (e) {
					log.error(e, 'Exception while executing query!');
					if (conn != null) conn.release();
				}
			});
		}
	}
})();

function handleServerError(e, req, res) {
	log.error(e, 'Exception while processing request!');
	res.status(500);	// internal server error
	res.send(e.message);
	res.end();
}

function initApi() {
	app.post('data/push', function (req, res) {
		try {
			var readings = req.body.readings;
			
			if (log.info())
				log.info('Received %d readings ...', readings.length);
			
			res.status(204);	// no content
			res.end();
		} catch (e) {
			handleServerError(e, req, res);
		}
	});
}

function initServer() {
	log.info('Initializing web server ...');
	
	initApi();
		
	var server = app.listen(config.server.port);
	
	log.info('================================================');
	log.info('Server running at http://localhost:%d', config.server.port);
	log.info('================================================');
	
	return server;
}

initServer();