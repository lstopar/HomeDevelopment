var express = require('express');
var session = require('express-session');
var ejs = require('ejs');
var bodyParser = require('body-parser');
var cookieParser = require('cookie-parser');

var SessionStore = require('./util/sessionstore.js');

var API_PATH = '/api';

var titles = {
	'': 'Index',
	'index.html': 'Index'
};

var app = express();

//=====================================================
// API
//=====================================================

function initApi() {
	// TODO
}

//=====================================================
// PAGE PREPARATION
//=====================================================

function accessControl(req, res, next) {
	var session = req.session;
	
	if (config.mode == 'debug') {
		initSession(session, {email: 'luka@lstopar.si', uid: 1, role: 'admin'});
		next();
	}
	else {
		var page = getRequestedPage(req);
		var dir = getRequestedPath(req);
		
		if (!isLoggedIn(session)) {
			if (log.debug())
				log.debug('Session data missing for page %s, dir %s ...', page, dir);
			
			if (dir == null || dir == '')
				redirectLogin(req, res);
			else {
				var isAjax = req.xhr;
				if (isAjax) {
					if (log.debug())
						log.debug('Session data missing for AJAX API call, blocking!')
					handleNoPermission(req, res);
				} else {
					redirect(res, '../login.html');
				}
			}
		} else {
			// the user already exists, check if they have permission for this page
			var email = session.user.email;
			var role = session.user.role;
			
			if (log.trace())
				log.trace('Checking permissions of user %s (%s) for page %s, dir %s ...', email, role, page, dir);
			
			var pageBlacklist = blacklist.pages[role];
			var dirBlacklist = blacklist.dirs[role];
			
			if (dir != null && dir != '') {
				if (log.trace())
					log.trace('Directory present, checking access to directory %s ...', dir);
				
				if (dir in dirBlacklist) {
					handleNoPermission(req, res);
					return;
				}
			} else if (page in pageBlacklist) {
				if (log.trace())
					log.trace('Directory not present, checking access to page %s ...', page);
				
				handleNoPermission(req, res);
				return;
			}
			
			if (log.trace())
				log.trace('User %s has permission for page %s!', email, page);

			next();
		}
	}
}

exports.init = function () {
	log.info('Initializing web server ...');
	
	app.set('view engine', 'ejs');
	
	app.use(session({ 
		unset: 'destroy',
		secret: config.session.secret,
		store: new SessionStore({ maxAge: config.session.maxIdleTime }),
		resave: false,
		saveUninitialized: true,
		cookie: { secure: false }
	}));
	
	app.use(API_PATH + '/', bodyParser.urlencoded({ extended: false }));
	app.use(API_PATH + '/', bodyParser.json());
	
	app.use(excludeDirs(['/js', '/css', '/lib', '/login'], excludeFiles(['/login.html', '/resetpassword.html'], accessControl)));
	
	app.get('/', prepPage('index'));
	app.get('/index.html', prepPage('index'));
	app.get('/login.html', prepPage('login'));
	app.get('/resetpassword.html', prepPage('resetpassword'));
	
	initApi();
	
	app.use(UI_PATH , express.static(path.join(__dirname, '../ui')));
	
	var server = app.listen(config.server.port);
	
	log.info('================================================');
	log.info('Server running at http://localhost:%d', config.server.port);
	log.info('Serving API at: %s', API_PATH);
	log.info('Max session idle time: %d', config.session.maxIdleTime);
	log.info('================================================');
}