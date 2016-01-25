module.exports = exports = [
	{
		"type": "DHT11",
		"sensors": [
			{
				"id": "lr-temp",
				"type": "temperature",
				"unit": "\u2103",
				"name": "Temperature",
				"description": "Temperature in the living room"
			},
			{
				"id": "lr-hum",
				"type": "humidity",
				"unit": "%",
				"name": "Humidity",
				"description": "Humidity in the living room"
			}
		],
		"configuration": {
			"pin": 4,
			"temperatureId": "lr-temp",
			"humidityId": "lr-hum",
			"timeout": 10000,
			"verbose": true
		}
	},
	{
		"type": "YL-40",
		"sensors": [
			{
				"id": "lr-lum",
				"type": "luminosity",
				"unit": "%",
				"name": "Luminosity",
				"description": "Luminosity in the living room",
			}
		],
		"configuration": {
			"inputs": [
				{
					"id": "lr-lum",
					"number": 0
				}
			],
			"verbose": true
		},
		"transform": function (vals) {
			vals['lr-lum'] = (255 - vals['lr-lum']) / 2.55;
		}
	}
]