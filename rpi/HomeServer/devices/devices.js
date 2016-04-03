var utils = require('../src/utils.js');

//=======================================================
// IDs
//=======================================================

var MOTION_SOFA_ID = 'motion-sofa';
var MOTION_TV_ID = 'motion-tv';
var TV_ID = 'lr-tv';
var LUMINOSITY_ID = 'lr-lum';
var TEMPERATURE_ID = 'lr-temp';
var HUMIDITY_ID = 'lr-hum';
var INDICATOR_LED_ID = 'led-test';
var LED_BLUE_ID = 'led-blue';
var LED_RED_ID = 'led-red';
var LED_GREEN_ID = 'led-green';
var AMBIENT_LIGHT_ID = 'light-ambient';
var MAIN_LIGHT_ID = 'light-main';
var BLINK_RGB_ID = 'rgb-blink';
var CYCLE_HSL_ID = 'hsl-cycle';

//=======================================================
// CONSTANTS
//=======================================================

var LUMINOSITY_THRESHOLD = 10;

//=======================================================
// OTHER VARIABLES
//=======================================================

var getValue = null;
var setValue = null;
var motionDetector = null;
var tv = null;

//=======================================================
// GET/SET FUNCTIONS
//=======================================================

function getLuminosity() {
	return getValue(LUMINOSITY_ID);
}

function getMotionTv() {
	return getValue(MOTION_TV_ID) == 1;
}

function getMotionSofa() {
	return getValue(MOTION_SOFA_ID) == 1;
}

function getMotion() {
	return getMotionTv() || getMotionSofa()	;
}

function isAmbientOn() {
	return getValue(AMBIENT_LIGHT_ID) == 1;
}

function ambientOn() {
	setValue({ sensorId: AMBIENT_LIGHT_ID, value: 1 });
}

function ambientOff() {
	setValue({ sensorId: AMBIENT_LIGHT_ID, value: 0 });
}

function ledStripOff() {
	// TODO
}

function lightsOff() {
	ambientOff();
	ledStripOff();
}

//=======================================================
// CHECKUP
//=======================================================

function periodicCheck() {
	try {
		log.info('Performing periodic check ...');
		
		if (motionDetector.timeSinceMotion() > 1000*60*30) {
			lightsOff();
		}
	} catch (e) {
		log.error(e, 'Exception while performing periodic check!');
	}
}

//=======================================================
// CONTROLLERS
//=======================================================

var MotionDetector = function () {
	var EMPTY_ROOM_THRESHOLD = 1000*60*30;	// 15 mins
	
	var lastMotionTime = new Date().getTime();
	
	var that = {
		onMotion: function (sensorId, motion) {
			if (getMotion()) {
				setValue({ sensorId: INDICATOR_LED_ID, value: 100 });
								
				if (getLuminosity() < LUMINOSITY_THRESHOLD && that.timeSinceMotion() > EMPTY_ROOM_THRESHOLD) {
					ambientOn();
				}
				
				lastMotionTime = new Date().getTime();
			} else {
				setValue({ sensorId: INDICATOR_LED_ID, value: 0 });
			}
		},
		timeSinceMotion: function () {
			return new Date().getTime() - lastMotionTime;
		}
	}
	
	return that;
}

var TvController = function () {
	var TV_SAMPLE_TIME = 5000;
	
	var isOn = false;
	var isReading = false;
	
	var push = function () {}
	
	function readTv() {
		if (isReading) return;
		
		isReading = true;
		
		try {
			if (log.trace())
				log.trace('Pinging TV ...');
			
			utils.ping('tv.home', function (isAlive) {
				try {
					if (log.trace())
						log.trace('Received response from TV: ' + isAlive);
					
					if (isOn != isAlive) {
						push({ id: TV_ID, value: isAlive ? 1 : 0 });
					}
				} catch (e) {
					log.error(e, 'Exception while processing TV state!');
				}
				
				isReading = false;
		    });
		} catch (e) {
			log.error(e, 'Exception while pinging TV!');
		}
	}
	
	var that = {
		onValue: function (_isOn) {
			if (log.trace())
				log.trace('Received TV value: ' + _isOn);
			
			if (_isOn != isOn) {
				if (log.debug())
					log.debug('TV status changed to ' + _isOn);
				
				if (_isOn) {
					if (isAmbientOn()) {
						ambientOff();
					}
				} else {
					if (getLuminosity() < LUMINOSITY_THRESHOLD && !isAmbientOn()) {
						ambientOn();
					}
				}
				
				isOn = _isOn;
			}
		},
		init: function () {
			log.info('Initializing TV ...');
			setInterval(readTv, TV_SAMPLE_TIME);
		},
		read: function (callback) {
			var result = {};
			result[TV_ID] = isOn ? 1 : 0;
			callback(undefined, result);
		},
		setPushCallback: function (callback) {
			push = callback;
		}
	}
	
	return that;
}

module.exports = exports = function (_getValue, _setValue) {
	getValue = _getValue;
	setValue = _setValue;
	
	motionDetector = MotionDetector();
	tv = TvController();
	
	setInterval(periodicCheck, 1000*60);
	
	function onValue(sensorId, value) {
		if (log.debug())
			log.debug('Received value in devices: { %s, %d }', sensorId, value);
		
		if (sensorId == MOTION_TV_ID || sensorId == MOTION_SOFA_ID) {
			motionDetector.onMotion(sensorId, value == 1);
		}
		else if (sensorId == TV_ID) {
			tv.onValue(value == 1);
		}
	}
	
	return {
		onValue: onValue,
		layout: [
		    {
		    	id: 'group-lights',
		    	img: 'img/bulb.svg',
		    	sensorIds: [
		    	    AMBIENT_LIGHT_ID,
		    	    LED_BLUE_ID,
		    	    LED_RED_ID,
		    	    LED_GREEN_ID,
		    	    BLINK_RGB_ID,
		    	    CYCLE_HSL_ID
		    	]
		    },
		    {
		    	id: 'group-motion',
		    	img: 'img/motion.svg',
		    	sensorIds: [
		    	    MOTION_TV_ID,
		    	    MOTION_SOFA_ID
		    	]
		    },
		    {
		    	id: 'group-ambient',
		    	img: 'img/ambient.svg',
		    	sensorIds: [
		    	    LUMINOSITY_ID,
		    	    TEMPERATURE_ID,
		    	    HUMIDITY_ID
		    	]
		    }
		],
		devices: [
			{
				type: 'Rf24',
				nodes: [
				    {
				    	id: 01,
				    	name: 'Arduino - TV',
				    	sensors: [
							{
								id: MOTION_TV_ID,
								internalId: 4,
								type: 'pir',
								unit: '',
								name: 'Motion TV',
								description: ''
							},
				    	    {
				    	    	id: INDICATOR_LED_ID,
				    	    	internalId: 3,
				    	    	type: 'dimmer',
				    	    	unit: '%',
				    	    	name: 'Brightness',
				    	    	description: 'Dimmer 1',
				    	    	hidden: true
				    	    },
				    	    {
				    	    	id: LED_BLUE_ID,
				    	    	internalId: 6,
				    	    	type: 'dimmer',
				    	    	unit: '%',
				    	    	name: 'LED blue',
				    	    	description: 'Dimmer 1'
				    	    },
				    	    {
				    	    	id: LED_RED_ID,
				    	    	internalId: 9,
				    	    	type: 'dimmer',
				    	    	unit: '%',
				    	    	name: 'LED red',
				    	    	description: 'Dimmer 1'
				    	    },
				    	    {
				    	    	id: LED_GREEN_ID,
				    	    	internalId: 5,
				    	    	type: 'dimmer',
				    	    	unit: '%',
				    	    	name: 'LED green',
				    	    	description: 'Dimmer 1'
				    	    },
				    	    {
				    	    	id: BLINK_RGB_ID,
				    	    	internalId: 15,
				    	    	type: 'actuator',
				    	    	unit: '',
				    	    	name: 'Blink RGB',
				    	    	description: ''
				    	    },
				    	    {
				    	    	id: CYCLE_HSL_ID,
				    	    	internalId: 14,
				    	    	type: 'actuator',
				    	    	unit: '',
				    	    	name: 'Cycle colors',
				    	    	description: ''
				    	    }
				    	]
				    },
				    {
				    	id: 02,
				    	name: 'Arduino - Sofa',
				    	sensors: [
							{
								id: LUMINOSITY_ID,
								internalId: 3,
								type: 'luminosity',
								unit: '%',
								name: 'Luminosity',
								description: '',
								transform: function (val) {
									return val / 2.55;
								}
							},
				    	    {
				    	    	id: MOTION_SOFA_ID,
				    	    	internalId: 4,
				    	    	type: 'pir',
				    	    	unit: '',
				    	    	name: 'Motion sofa',
				    	    	description: ''
				    	    }/*,
				    	    {
				    	    	id: AMBIENT_LIGHT_ID,
				    	    	internalId: 6,
				    	    	type: 'actuator',
				    	    	unit: '',
				    	    	name: 'Ambient light',
				    	    	description: ''
				    	    }*/
				    	]
				    }
				],
				configuration: {
					pinCE: 25,		// RPI_V2_GPIO_P1_22	// TODO the pins are hardcoded in C++
					pinCSN: 8,		// RPI_V2_GPIO_P1_24
					id: 00,
					verbose: true
				}
			},
			{
				type: 'EnOcean',
				nodes: [
//				    {
//				    	id: 'nodon-module',
//				    	internalId: 3, // TODO
//				    	name: 'NodOn Relay',
//				    	type: 'D2-01-xx',
//				    	sensors: [
//							{
//								id: MAIN_LIGHT_ID,
//								internalId: 0,
//								type: 'actuator',
//								unit: '',
//								name: 'Main Light',
//								description: ''
//							},
//							{
//								id: AMBIENT_LIGHT_ID,
//								internalId: 1,
//								type: 'actuator',
//								unit: '',
//								name: 'Ambient Light',
//								description: ''
//							}
//				    	]
//				    }
				],
				configuration: {
					storageFile: 'config/enocean.txt',
					serialPort: '/dev/ttyUSB0',
					verbose: true
				}
			},
	  		{
				type: "DHT11",
				sensors: [
					{
						id: TEMPERATURE_ID,
						type: "temperature",
						unit: "\u2103",
						name: "Temperature",
						description: "Temperature in the living room"
					},
					{
						id: HUMIDITY_ID,
						type: "humidity",
						unit: "%",
						name: "Humidity",
						description: "Humidity in the living room"
					}
				],
				configuration: {
					pin: 4,
					temperatureId: TEMPERATURE_ID,
					humidityId: HUMIDITY_ID,
					timeout: 10000,
					verbose: true
				}
			},
			{
				type: 'virtual',
				sensors: [
				    {
				    	id: TV_ID,
				    	type: 'binary',
				    	img: 'img/tv.svg',
				    	unit: '',
				    	name: 'TV',
				    	description: 'Television',
				    	controller: tv
				    }      
				]
			}
		]
	} 
}