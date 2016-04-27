#include "arduino_home.h"

const int TRgbStrip::UPDATE_INTERVAL = 500;

const int TRgbStrip::PIN_RED_N = 0;
const int TRgbStrip::PIN_GREEN_N = 1;
const int TRgbStrip::PIN_BLUE_N = 2;

TRgbStrip::TRgbStrip(const int& pinR, const int& pinG, const int& pinB):
		mode(TRgbMode::rmDefault),
		blinkPinN(0),
		currHue(0),
		iteration(0) {
	pins[0] = pinR;
	pins[1] = pinG;
	pins[2] = pinB;
	reset();
}

void TRgbStrip::update() {
	if (iteration++ % UPDATE_INTERVAL != 0) { return; }

	if (mode == TRgbMode::rmBlink) {
		pinVals[blinkPinN]++;
		if (pinVals[blinkPinN] > 255) {
			reset(false, true);
			blinkPinN++;
			blinkPinN %= 3;
		}

		setColor(blinkPinN, pinVals[blinkPinN], false);
	} else if (mode == TRgbMode::rmCycleHsv) {
		currHue = (currHue + 1) % 360;
		int r, g, b;
		hsl2rgb(currHue, 1, 1, r, g, b);
		setRed(r, false);
		setBlue(b, false);
		setGreen(g, false);
	}
}

void TRgbStrip::blink() {
	reset();
	mode = TRgbMode::rmBlink;
	blinkPinN = 0;
}

void TRgbStrip::cycleHsv() {
	reset();
	mode = TRgbMode::rmCycleHsv;
	currHue = 0;
}

void TRgbStrip::reset(const bool& resetModes, const bool& resetPins) {
	if (resetModes) {
		mode = TRgbMode::rmDefault;
	}

	if (resetPins) {
		for (int pinN = 0; pinN < 3; pinN++) {
			setColor(pinN, 0, false);
		}
	}
}

int TRgbStrip::getColor(const int& colorN) const {
	return pinVals[colorN];
}

void TRgbStrip::setColor(const int& colorN, const int& val, const bool& resetModes) {
	if (resetModes) {
		reset(true, false);
	}

	pinVals[colorN] = val;
	analogWrite(pins[colorN], pinVals[colorN]);
}

void TRgbStrip::hsl2rgb(const float& _h, const float& _s, const float& _l,
		int& r, int& g, int& b) {
	float h = fmod(_h,360); // cycle H around to 0-360 degrees
	h = 3.14159*h/(float)180; // Convert to radians.
	float s = _s>0?(_s<1?_s:1):0; // clamp S and I to interval [0,1]
	float l = _l>0?(_l<1?_l:1):0;

	// Math! Thanks in part to Kyle Miller.
	if (h < 2.09439) {
		r = 255*l/3*(1+s*cos(h)/cos(1.047196667-h));
		g = 255*l/3*(1+s*(1-cos(h)/cos(1.047196667-h)));
		b = 255*l/3*(1-s);
	} else if (h < 4.188787) {
		h = h - 2.09439;
		g = 255*l/3*(1+s*cos(h)/cos(1.047196667-h));
		b = 255*l/3*(1+s*(1-cos(h)/cos(1.047196667-h)));
		r = 255*l/3*(1-s);
	} else {
		h = h - 4.188787;
		b = 255*l/3*(1+s*cos(h)/cos(1.047196667-h));
		r = 255*l/3*(1+s*(1-cos(h)/cos(1.047196667-h)));
		g = 255*l/3*(1-s);
	}
}


TManualDimmer::TManualDimmer(const int& _VOutPin, const int& _ReadPin, const int& _PwmPin,
			const int& _MnVal, const int& _MxVal):
		vOutPin(_VOutPin),
		readPin(_ReadPin),
		pwmPin(_PwmPin),
		mnVal(_MnVal),
		mxVal(_MxVal),
		currVal(0) {}


void TManualDimmer::init() {
	pinMode(vOutPin, OUTPUT);
	pinMode(readPin, INPUT);
	pinMode(pwmPin, OUTPUT);

	update();
}

void TManualDimmer::update() {
	float perc = float(readInput() - mnVal) / (mxVal - mnVal);

	if (perc < 0) perc = 0;
	if (perc > 1) perc = 1;

	const int outVal = int(perc*255);

	if (abs(currVal - outVal) > THRESHOLD ||
			(outVal == 255 && currVal != 255) ||
			(outVal == 0 && currVal != 0)) {
		currVal = outVal;
		analogWrite(pwmPin, currVal);
	}
}

int TManualDimmer::readInput() const {
	digitalWrite(vOutPin, HIGH);
	int result = analogRead(readPin);
	digitalWrite(vOutPin, LOW);
	return result;
}

TManualSwitch::TManualSwitch(const int& _vOutPin, const int& _readPin,
		const int& _switchPin):
	vOutPin(_vOutPin),
	readPin(_readPin),
	outputPin(_switchPin),
	inputOn(false),
	outputOn(false),
	onStateChanged(nullptr) {}

void TManualSwitch::init() {
	pinMode(vOutPin, OUTPUT);
	pinMode(readPin, INPUT);
	pinMode(outputPin, OUTPUT);

	digitalWrite(outputPin, LOW);
}

void TManualSwitch::update() {
	const bool newInputOn = readSwitch();

	if (newInputOn != inputOn) {
		inputOn = newInputOn;
		toggle();
	}
}

void TManualSwitch::on() {
	setOutput(true);
}

void TManualSwitch::off() {
	setOutput(false);
}

void TManualSwitch::toggle() {
	setOutput(!isOn());
}

void TManualSwitch::setOutput(const bool& on) {
	if (on != outputOn) {
		outputOn = on;
		digitalWrite(outputPin, isOn() ? LOW : HIGH);	// TODO change LOW and HIGH, check the wiring
		if (onStateChanged != nullptr) {
			onStateChanged(outputOn);
		}
	}
}

bool TManualSwitch::readSwitch() {
	digitalWrite(vOutPin, HIGH);
	delay(3);
	const int result = digitalRead(readPin);
	digitalWrite(vOutPin, LOW);
	return result == HIGH;
}

///////////////////////////////////////
// Digital PIR sensor
TDigitalPir::TDigitalPir(const int& _readPin):
		readPin(_readPin),
		isMotionDetected(false),
		onStateChanged(nullptr) {}

void TDigitalPir::init() {
	pinMode(readPin, INPUT);
}

void TDigitalPir::update() {
	const bool isOn = readInput();

	if (isOn != isMotionDetected) {
		isMotionDetected = isOn;

		if (onStateChanged != nullptr) {
			onStateChanged(isMotionDetected);
		}
	}
}

bool TDigitalPir::readInput() {
	const int val = digitalRead(readPin);
	//Serial.println(val);
	return val == HIGH;
}

///////////////////////////////////////
// Analog PIR sensor
TAnalogPir::TAnalogPir(const int& readPin, const uint64_t& _onTime, const int& _threshold):
		TDigitalPir(readPin),
		threshold(_threshold),
		onTime(_onTime),
		lastOnTime(0) {}

bool TAnalogPir::readInput() {
	const int pirVal = analogRead(readPin);
	const bool isOn = pirVal >= threshold;

	if (isOn) {
		// the motion is detected
		lastOnTime = millis();
		return true;
	}

	// return true if the last motion was less than
	// the time threshold ago
	return millis() - lastOnTime < onTime;
}

TRadioMsg::TRadioMsg():
		addr(0),
		type(0),
		payload(NULL),
		len(0) {}

TRadioMsg::TRadioMsg(const uint16_t& _addr, const unsigned char& _type,
			const char* _payload, const int& _len):
		addr(_addr),
		type(_type),
		payload(NULL),
		len(_len) {
	if (len > 0) {
		payload = new char[len];
		memcpy(payload, _payload, len);
	}
}

TRadioMsg::TRadioMsg(const TRadioMsg& msg):
		addr(msg.addr),
		type(msg.type),
		payload(NULL),
		len(msg.len) {
	if (len > 0) {
		payload = new char[len];
		memcpy(payload, msg.payload, len);
	}
}

TRadioMsg::~TRadioMsg() {
	cleanUp();
}

TRadioMsg& TRadioMsg::operator =(const TRadioMsg& msg) {
	if (this != &msg) {
		cleanUp();

		addr = msg.addr;
		type = msg.type;
		len = msg.len;

		if (len > 0) {
			payload = new char[len];
			memcpy(payload, msg.payload, len);
		}
	}

	return *this;
}

void TRadioMsg::cleanUp() {
	if (payload != NULL) {
		delete[] payload;
	}
}


///////////////////////////////////////
// Radio wrapper
TRf24Wrapper::TRf24Wrapper(const uint16_t _address, RF24& _radio, RF24Network& _network):
		address(_address),
		radio(_radio),
		network(_network),
		receiveQ(),
		processGet(NULL),
		processSet(NULL) {}

void TRf24Wrapper::init() {
	TRadioProtocol::InitRadio(radio, network, address, RF24_PA_MAX);
}

void TRf24Wrapper::update() {
	network.update();

	RF24NetworkHeader header;
	while (network.available()) {
		// look at the header and read the message
		network.peek(header);

		const uint16_t& from = header.from_node;
		const unsigned char& type = header.type;

		if (TRadioProtocol::HasPayload(type)) {
			network.read(header, receivePayload, PAYLOAD_LEN);
		}
		else {
			network.read(header, NULL, 0);
		}

		// acknowledge
		if (type != REQUEST_ACK) {
			_send(from, REQUEST_ACK);
		}

		if (type == REQUEST_GET || type == REQUEST_SET) {
			receiveQ.push(TRadioMsg(from, type, receivePayload, PAYLOAD_LEN));
		}
		else if (type == REQUEST_PING) {
			Serial.println("Received ping, ponging ...");
			_send(from, REQUEST_PONG);
		}
		else {
			Serial.print("Received unsupported message type: ");
			Serial.println(type);
		}
	}

	if (!receiveQ.empty()) {
		while (!receiveQ.empty()) {
			const TRadioMsg& msg = receiveQ.front();
			processMsg(msg);
			receiveQ.pop();
		}
	}
}

bool TRf24Wrapper::push(const uint16_t& to, const std::vector<TRadioValue>& values) {
	if (values.empty()) { return true; }

	const int size = values.size();
	if (size > VALS_PER_PAYLOAD) {
		bool success = true;

		const int nMsgs = ceil(double(values.size()) / double(VALS_PER_PAYLOAD));
		for (int msgN = 0; msgN < nMsgs; msgN++) {
			const int startN = msgN*VALS_PER_PAYLOAD;
			const int endN = std::min(size, (msgN+1)*VALS_PER_PAYLOAD);

			std::vector<TRadioValue> subVals(values.begin() + startN, values.begin() + endN);
			success &= push(to, subVals);
		}

		return success;
	}
	else {
		char payload[VALS_PER_PAYLOAD];
		TRadioProtocol::genPushPayload(values, payload);
		return _send(to, REQUEST_PUSH, payload, PAYLOAD_LEN);
	}
}

bool TRf24Wrapper::push(const uint16_t& to, const TRadioValue& value) {
	std::vector<TRadioValue> valueV;
	valueV.push_back(value);
	return push(to, valueV);
}

bool TRf24Wrapper::_send(const uint16_t& to, const unsigned char& type, const char* payload,
			const int& payloadLen) {
	Serial.println("Sending message ...");

	RF24NetworkHeader header(to, type);
	const bool success = network.write(header, payload, payloadLen);

	if (!success) {
		Serial.println("Failed to send message!");
	}

	return success;
}

void TRf24Wrapper::processMsg(const TRadioMsg& msg) const {
	const uint16_t& node = msg.getAddr();
	const unsigned char& type = msg.getType();

	switch (type) {
	case REQUEST_GET: {
		Serial.println("Received GET request ...");

		std::vector<char> valueIdV;
		TRadioProtocol::parseGetPayload(msg.getPayload(), valueIdV);

		if (processGet != NULL) {
			processGet(node, valueIdV);
		}
		break;
	}
	case REQUEST_SET: {
		Serial.println("Received SET ...");

		std::vector<TRadioValue> valueV;
		TRadioProtocol::parseSetPayload(msg.getPayload(), valueV);

		if (processSet != NULL) {
			processSet(node, valueV);
		}
		break;
	}
	default: {
		Serial.print("Unknown message type ");
		Serial.print(type);
		Serial.print(" from node ");
		Serial.println(node);
	}
	}
}
