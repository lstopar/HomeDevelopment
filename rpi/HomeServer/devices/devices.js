module.exports = exports = [
	/*{
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
	},*/
	{
		type: 'Rf24',
		nodes: [
		    {
		    	id: 01,
		    	name: 'Arduino - sofa',
		    	sensors: [
		    	    {
		    	    	id: 'light-sofa',
		    	    	internalId: 3,
		    	    	type: 'dimmer',
		    	    	unit: '%',
		    	    	name: 'Brightness',
		    	    	description: 'Dimmer 1'
		    	    }          
		    	]
		    },
		    {
		    	id: 04,
		    	name: 'Arduino - motion',
		    	sensors: [
		    	    {
		    	    	id: 'motion-lr',
		    	    	internalId: 4,
		    	    	type: 'pir',
		    	    	unit: '[0/1]',
		    	    	name: 'Motion living room',
		    	    	description: ''
		    	    }
		    	]
		    }
		],
		configuration: {
			pinCE: 25,		// RPI_V2_GPIO_P1_22
			pinCSN: 8,		// RPI_V2_GPIO_P1_24
			id: 00,
			verbose: true
		}
	}
	
]