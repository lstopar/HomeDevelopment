module.exports = exports = [
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
	},
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
		    }    
		],
		configuration: {
			pinCE: 25,
			pinCSN: 8,
			verbose: true
		}
	}
]