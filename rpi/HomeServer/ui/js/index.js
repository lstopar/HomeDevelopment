$(document).ready(function () {
	function onReading(reading) {
		var id = reading.id;
		var val = reading.value;
		var type = reading.type;
				
		if (type == 'dimmer') {
			var slider = $('#range-' + id);
			slider.slider('setValue', tuUiPrecision(val, type));
		} else if (type == 'actuator') { 
			var chk = $('#chk-' + id);
			chk.attr('checked', val == 1);
		} else {
			var valSpan = $('#span-val-' + id);
			valSpan.html(tuUiPrecision(val, type));
		}
	}
	
	function onNodeEvent(event) {
		var nodeId = event.id;
		var connected = event.connected;
		
		var span = $('#span-status-node-' + nodeId);
		
		span.removeClass('label-danger');
		span.removeClass('label-success');
		
		if (connected) {
			span.addClass('label-success');
			span.html('Connected');
		} else {
			span.addClass('label-danger');
			span.html('Disconnected');
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
			} else if (msg.type == 'nodeEvent') {
				onNodeEvent(msg.content);
			} else {
				alert('Unknown message: ' + msgStr.data);
			}
		};
	}
	
	initWs();
	$('.nav-pills a')[0].click();
});