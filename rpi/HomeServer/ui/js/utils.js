function tuUiPrecision(val, type) {
	if (type == 'luminocity') {
		return (val*100).toFixed();
	} else {
		return val.toFixed();
	}
}