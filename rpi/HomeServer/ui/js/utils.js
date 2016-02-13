function tuUiPrecision(val, type) {
	return parseInt(val.toFixed());
}

function createSlider(sensorId, min, max, val) {
	$('#range-' + sensorId).slider({
		min: min,
		max: max,
		step: 1,
		value: val
	}).on('slideStop', function (event) {
		var value = event.value;
		
		$.ajax('api/set', {
			method: 'POST',
			dataType: 'json',
			data: {
				id: sensorId,
				value: parseInt(value)
			},
			success: function () {},
			error: handleAjaxError()
		})
	});
}

function handleAjaxError(alertField, callback) {
	return function (xhr, status, err) {
		if (xhr.readyState == 0) {
			console.log('Ajax error with request not initialized!');
		} else {
			if (xhr.status == 400 && alertField != null) {
				showAlert($('#alert-holder'), alertField, 'alert-danger', xhr.responseText, null, false);
			} else {
				alert(xhr.responseText);
			}
			
			if (callback != null)
				callback();
		}
	}
}