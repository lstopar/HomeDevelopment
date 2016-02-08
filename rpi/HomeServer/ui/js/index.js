$(document).ready(function () {
	function onReading(reading) {
		var id = reading.id;
		var val = reading.value;
		var type = reading.type;
		
		var thumbnail = $('#thumb-' + id);
		
		if (type != 'dimmer') {
			var valSpan = thumbnail.find('#span-val-' + id);
			valSpan.html(tuUiPrecision(val, type));
		} else {
			var slider = thumbnail.find('#range-' + id);
			slider.slider('setValue', tuUiPrecision(val, type));
		}
	}
	
	function getWsUrl() {
		var result;
		var loc = window.location;
		
		if (loc.protocol === "https:") {
		    result = "wss:";
		} else {
		    result = "ws:";
		}
		
		var path = loc.pathname;
		path = path.substring(0, path.lastIndexOf('/')) + '/ws';
		
		result += "//" + loc.host + path;
		return result;
	}
	
	function initWs() {
		var address = getWsUrl();
		
		var isDrawing = false;
		
		console.log('Connecting websocket to address: ' + address); 
		var ws = new WebSocket(address);
		
		ws.onopen = function () {
   			console.log('Web socket connected!');
		};
		
		ws.onerror = function (e) {
			console.log('Web socket error: ' + e.message);
			alert('Web socket error!');
		};
		
		ws.onmessage = function (msgStr) {
			var msg = JSON.parse(msgStr.data);
			
			if (msg.type == 'reading') {
				onReading(msg.content);
			} else {
				alert('Unknown message: ' + msgStr.data);
			}
		};
	}
	
	initWs();
	$('.nav-pills a')[0].click();
});