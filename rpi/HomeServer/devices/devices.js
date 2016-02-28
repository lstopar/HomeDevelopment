var MOTION_SOFA_ID = 'motion-sofa';
var MOTION_TV_ID = 'motion-tv';
var LUMINOSITY_ID = 'lum-lr';
var INDICATOR_LED_ID = 'led-test';
var LED_BLUE_ID = 'led-blue';
var LED_RED_ID = 'led-red';
var LED_GREEN_ID = 'led-green';
var AMBIENT_LIGHT_ID = 'light-door';
var BLINK_RGB_ID = 'rgb-blink';
var CYCLE_HSL_ID = 'hsl-cycle';

var getValue = null;
var setValue = null;
var motionDetector = null;

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

function ambientOn() {
	setValue(AMBIENT_LIGHT_ID, 1);
}

function ambientOff() {
	setValue(AMBIENT_LIGHT_ID, 0);
}

function ledStripOff() {
	// TODO
}

//=======================================================
// CHECKUP
//=======================================================

function periodicCheck() {
	try {
		log.info('Performing periodic check ...');
		
		if (motionDetector.timeSinceMotion() > 1000*60*30) {
			ambientOff();
			ledStripOff();
		}
	} catch (e) {
		log.error(e, 'Exception while performing periodic check!');
	}
}

//=======================================================
// MOTION
//=======================================================

var MotionDetector = function () {
	var EMPTY_ROOM_THRESHOLD = 1000*60*30;
	var LUMINOSITY_THRESHOLD = 10;
	
	var lastMotionTime = new Date().getTime();
	
	var that = {
		onMotion: function (sensorId, motion) {
			if (getMotion()) {
				setValue(INDICATOR_LED_ID, 100);
								
				if (getLuminosity() < LUMINOSITY_THRESHOLD && that.timeSinceMotion() > EMPTY_ROOM_THRESHOLD) {
					ambientOn();
				}
				
				lastMotionTime = new Date().getTime();
			} else {
				setValue(INDICATOR_LED_ID, 0);
			}
		},
		timeSinceMotion: function () {
			return new Date().getTime() - lastMotionTime;
		}
	}
	
	return that;
}

module.exports = exports = function (_getValue, _setValue) {
	getValue = _getValue;
	setValue = _setValue;
	
	motionDetector = MotionDetector();
	
	setInterval(periodicCheck, 1000*60);
	
	function onValue(sensorId, value) {
		if (sensorId == MOTION_TV_ID || sensorId == MOTION_SOFA_ID) {
			motionDetector.onMotion(sensorId, value == 1);
		}
	}
	
	return {
		onValue: onValue,
		layout: [
		    {
		    	id: 'group-lights',
		    	img: 'img/bulb.png',
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
		    	img: 'img/bulb.png',
		    	sensorIds: [
		    	    MOTION_TV_ID,
		    	    MOTION_SOFA_ID
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
				    	    	description: 'Dimmer 1'
				    	    },
				    	    {
				    	    	id: LED_BLUE_ID,
				    	    	internalId: 5,
				    	    	type: 'dimmer',
				    	    	unit: '%',
				    	    	name: 'LED blue',
				    	    	description: 'Dimmer 1'
				    	    },
				    	    {
				    	    	id: LED_RED_ID,
				    	    	internalId: 6,
				    	    	type: 'dimmer',
				    	    	unit: '%',
				    	    	name: 'LED red',
				    	    	description: 'Dimmer 1'
				    	    },
				    	    {
				    	    	id: LED_GREEN_ID,
				    	    	internalId: 9,
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
								name: 'Luminosity living room',
								description: '',
								transform: function (val) {
									return val / 10.23;
								}
							},
				    	    {
				    	    	id: MOTION_SOFA_ID,
				    	    	internalId: 4,
				    	    	type: 'pir',
				    	    	unit: '',
				    	    	name: 'Motion living room',
				    	    	description: ''
				    	    },
				    	    {
				    	    	id: AMBIENT_LIGHT_ID,
				    	    	internalId: 6,
				    	    	type: 'actuator',
				    	    	unit: '',
				    	    	name: 'Ambient light',
				    	    	description: ''
				    	    }
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
				type: "DHT11",
				sensors: [
					{
						id: "lr-temp",
						type: "temperature",
						unit: "\u2103",
						name: "Temperature",
						description: "Temperature in the living room"
					},
					{
						id: "lr-hum",
						type: "humidity",
						unit: "%",
						name: "Humidity",
						description: "Humidity in the living room"
					}
				],
				configuration: {
					pin: 4,
					temperatureId: "lr-temp",
					humidityId: "lr-hum",
					timeout: 10000,
					verbose: true
				}
			}
		]
	} 
}