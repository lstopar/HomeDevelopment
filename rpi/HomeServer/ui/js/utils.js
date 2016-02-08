function tuUiPrecision(val, type) {
	return val.toFixed();
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