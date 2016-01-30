var express = require('express');
var session = require('express-session');
var favicon = require('express-favicon');
var ejs = require('ejs');
var bodyParser = require('body-parser');
var cookieParser = require('cookie-parser');
var path = require('path');

var config = require('../config.js');
var utils = require('./utils.js');

var WebSocketWrapper = require('./utils/websockets.js').WebSocketWrapper;
var SessionStore = require('./utils/sessionstore.js');

var TITLES = {
	'/': 'Index',
	'index.html': 'Index'
};

var UI_PATH = '/';
var API_PATH = '/api';
var WS_PATH = '/ws';

var titles = {
	'': 'Index',
	'index.html': 'Index'
};

var app = express();
var sensors;

//=====================================================
//UTILITY METHODS
//=====================================================

function getRequestedPage(req) {
	return req.path.split('/').pop();
}

//=====================================================
// API
//=====================================================

function initApi() {
	// TODO
}

//=====================================================
// PAGE PREPARATION
//=====================================================

function getPageOpts(req, res) {
	var page = getRequestedPage(req);
	
	var opts = {
		utils: utils,
		sensors: sensors,
		subtitle: TITLES[page]
	};
	
	return opts;
}

function prepPage(page) {
	return function(req, res) {
		res.render(page, getPageOpts(req, res));
	}
}

//=====================================================
// INITIALIZATION
//=====================================================

function initServer(sessionStore, parseCookie) {
	log.info('Initializing web server ...');
	
	var sess = session({ 
		unset: 'destroy',
		store: sessionStore,
		cookie: { maxAge: 1000*60*60*24 },	// the cookie will last for 1 day
		resave: true,
		saveUninitialized: true
	});
	
	app.set('view engine', 'ejs');
	
	app.use(parseCookie);
	app.use(sess);
	app.use(API_PATH + '/', bodyParser.urlencoded({ extended: false }));
	app.use(API_PATH + '/', bodyParser.json());
		
	app.get('/', prepPage('index'));
	app.get('/index.html', prepPage('index'));
	
	initApi();
	
	app.use(UI_PATH , express.static(path.join(__dirname, '../ui')));
	app.use(favicon(path.join(__dirname, '../ui/img/favicon.ico')));
	
	var server = app.listen(config.server.port);
	
	log.info('================================================');
	log.info('Server running at http://localhost:%d', config.server.port);
	log.info('Serving API at: %s', API_PATH);
	log.info('================================================');
	
	return server;
}

exports.init = function (opts) {
	if (opts.sensors == null) throw new Error('Sensors missing when initializing the server!');
	
	sensors = opts.sensors;
	
	var sessionStore = new SessionStore();
	var parseCookie = cookieParser(config.session.secret);
	
	var server = initServer(sessionStore, parseCookie);
	
	var ws = WebSocketWrapper({
		server: server,
		sessionStore: sessionStore,
		parseCookie: parseCookie,
		webSocketPath: WS_PATH,
		onConnected: function (wsId, sessionId, session) {
			if (log.debug())
				log.debug('New web socket connected %d', wsId);
		}
	});
	
	sensors.onValueReceived(function (val) {
		ws.distribute(JSON.stringify({
			type: 'reading',
			content: val
		}));
	});
	
	log.info('Server initialized!');
}