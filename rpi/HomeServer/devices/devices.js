var MOTION_ID = 'motion-lr';
var LUMINOSITY_ID = 'lum-lr';
var DIMMER_ID = 'light-sofa';
var LED_BLUE_ID = 'led-blue';
var LED_RED_ID = 'led-red';
var LED_GREEN_ID = 'led-green';
var BLINK_RGB_ID = 'rgb-blink';

module.exports = exports = function (setValue) {
	return {
		onValue: function (sensorId, value) {
			if (sensorId == MOTION_ID) {
				setValue(DIMMER_ID, value == 1 ? 100 : 0);
			}
		},
		layout: [
		    {
		    	id: 'group-rgb',
		    	img: 'img/bulb.png',
		    	sensorIds: [
		    	    LED_BLUE_ID,
		    	    LED_RED_ID,
		    	    LED_GREEN_ID,
		    	    BLINK_RGB_ID
		    	]
		    }
		],
		devices: [
			{
				type: 'Rf24',
				nodes: [
				    {
				    	id: 01,
				    	name: 'Arduino - sofa',
				    	sensors: [
				    	    {
				    	    	id: DIMMER_ID,
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
				    	    }
				    	]
				    },
				    {
				    	id: 02,
				    	name: 'Arduino - motion',
				    	sensors: [
							{
								id: LUMINOSITY_ID,
								internalId: 3,
								type: 'luminosity',
								unit: '%',
								name: 'Luminosity living room',
								description: ''
							},
				    	    {
				    	    	id: MOTION_ID,
				    	    	internalId: 4,
				    	    	type: 'pir',
				    	    	unit: '',
				    	    	name: 'Motion living room',
				    	    	description: ''
				    	    }/*,
				    	    {
				    	    	id: 'motion-val',
				    	    	internalId: 5,
				    	    	type: 'pir',
				    	    	unit: '',
				    	    	name: 'Motion living room',
				    	    	description: ''
				    	    },*/
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