#include "protocol.h"

const int TRadioValue::BYTES = 2;

void TRadioValue::WriteToBuff(char* Buff) const {
	Buff[0] = ValId;
	Buff[1] = Val & 0xFF;
}

void TRadioValue::ReadFromBuff(const char* Buff) {
	ValId = Buff[0];
	Val = (int) Buff[1];
}

bool TRadioProtocol::IsValidType(const unsigned char& Type) {
	return Type == REQUEST_PING ||
			Type == REQUEST_CHILD_CONFIG ||
			Type == REQUEST_GET ||
			Type == REQUEST_PUSH ||
			Type == REQUEST_SET;
}

bool TRadioProtocol::HasPayload(const unsigned char& Type) {
	return Type != REQUEST_PING && Type != REQUEST_CHILD_CONFIG;
}

#ifndef ARDUINO
void TRadioProtocol::ParseGetPayload(const TMem& Payload, TChV& ValIdV) {
	EAssertR(Payload.Len() == PAYLOAD_LEN, "ParseGetPayload: invalid payload length: " + TInt::GetStr(Payload.Len()));
#else
int TRadioProtocol::parseGetPayload(const char* Payload, char* ValIdV) {
#endif
	const int Vals = (int) Payload[0];

#ifndef ARDUINO
	ValIdV.Gen(Vals);
#endif

	for (int ValN = 0; ValN < Vals; ValN++) {
		ValIdV[ValN] = Payload[ValN+1];
	}

#ifdef ARDUINO
	return Vals;
#endif
}


#ifndef ARDUINO
void TRadioProtocol::ParseSetPayload(const TMem& Payload, TVec<TRadioValue>& ValV) {
	EAssertR(Payload.Len() == PAYLOAD_LEN, "ParseSetPayload: invalid payload length: " + TInt::GetStr(Payload.Len()));
#else
int TRadioProtocol::parseSetPayload(const char* Payload, TRadioValue* ValV) {
#endif
	const int Vals = (int) Payload[0];

#ifndef ARDUINO
	ValV.Gen(Vals);
#endif

	for (int ValN = 0; ValN < Vals; ValN++) {
		TRadioValue& Val = ValV[ValN];
		Val.ReadFromBuff(&Payload[ValN*TRadioValue::BYTES + 1]);
	}

#ifdef ARDUINO
	return Vals;
#endif
}

#ifndef ARDUINO
void TRadioProtocol::GenGetPayload(const TChV& ValIdV, TMem& Payload) {
	if (Payload.Len() != PAYLOAD_LEN) { Payload.Gen(PAYLOAD_LEN); }
	const int& Vals = ValIdV.Len();
#else
void TRadioProtocol::genGetPayload(const int* ValIdV, const int& Vals,
		char* Payload) {
#endif

	Payload[0] = Vals;
	for (int ValN = 0; ValN < Vals; ValN++) {
		Payload[ValN+1] = ValIdV[ValN];
	}
}

#ifndef ARDUINO
void TRadioProtocol::GenSetPayload(const TVec<TRadioValue>& ValV, TMem& Payload) {
	if (Payload.Len() != PAYLOAD_LEN) { Payload.Gen(PAYLOAD_LEN); }
	const int& Vals = ValV.Len();
	EAssert(Vals*TRadioValue::BYTES + 1 <= PAYLOAD_LEN);
#else
void TRadioProtocol::genSetPayload(const TRadioValue* ValV, const int& Vals,
			char* Payload) {
#endif

	Payload[0] = (char) Vals;
	for (int ValN = 0; ValN < Vals; ValN++) {
		const TRadioValue& Val = ValV[ValN];
		Val.WriteToBuff(&Payload[ValN*TRadioValue::BYTES + 1]);
	}
}

void TRadioProtocol::InitRadio(RF24& Radio, RF24Network& Network, const uint16_t& Addr,
		const rf24_pa_dbm_e& Power) {
	Radio.begin();
	Radio.setAutoAck(true);
	Radio.setRetries(20, 30);
	Radio.setDataRate(RF24_2MBPS);
	Radio.setPALevel(Power);	// set power to high for better range
	Radio.setCRCLength(rf24_crclength_e::RF24_CRC_8);
	delay(5);
	Network.begin(COMM_CHANNEL, Addr);
	Radio.printDetails();
}


#ifdef ARDUINO

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
		digitalWrite(outputPin, isOn() ? HIGH : LOW);
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
		motionDetected(false),
		onStateChanged(nullptr) {}

void TDigitalPir::init() {
	pinMode(readPin, INPUT);
}

void TDigitalPir::update() {
	const bool newStatus = readInput();

	if (newStatus != motionDetected) {
		motionDetected = newStatus;

		if (onStateChanged != nullptr) {
			onStateChanged(motionDetected);
		}
	}
}

bool TDigitalPir::readInput() {
	return digitalRead(readPin) == HIGH;
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
	else {
		// motion is not detected, check if less time than the threshold has elapsed
		if (millis() - lastOnTime > onTime) {
			return true;
		}

		return false;
	}
}

#endif
