#include "protocol.h"

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
void TRadioProtocol::ParseGetPayload(const TMem& Payload, TIntV& ValIdV) {
	EAssertR(Payload.Len() == PAYLOAD_LEN, "ParseGetPayload: invalid payload length: " + TInt::GetStr(Payload.Len()));
#else
int TRadioProtocol::parseGetPayload(const char* Payload, int* ValIdV) {
#endif
	const int Vals = (int) Payload[0];

#ifndef ARDUINO
	ValIdV.Gen(Vals);
#endif

	for (int ValN = 0; ValN < Vals; ValN++) {
		ValIdV[ValN] = (int) Payload[ValN+1];
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
	const int Vals = Payload[0];

#ifndef ARDUINO
	ValV.Gen(Vals);
#endif

	const int ValLen = sizeof(TRadioValue);
	for (int ValN = 0; ValN < Vals; ValN++) {
		memcpy(&ValV[ValN], &Payload[ValN*ValLen + 1], sizeof(TRadioValue));
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
	EAssert(Vals*sizeof(TRadioValue) + 1 <= PAYLOAD_LEN);
#else
void TRadioProtocol::genSetPayload(const TRadioValue* ValV, const int& Vals,
			char* Payload) {
#endif

	Payload[0] = (char) Vals;
	const int ValLen = sizeof(TRadioValue);
	for (int ValN = 0; ValN < Vals; ValN++) {
		memcpy(&Payload[ValN*ValLen + 1], &ValV[ValN], sizeof(TRadioValue));
	}
}

void TRadioProtocol::InitRadio(RF24& Radio, RF24Network& Network, const uint16_t& Addr,
		const rf24_pa_dbm_e& Power) {
	Radio.begin();
	Radio.setAutoAck(true);
	Radio.setRetries(15, 15);
	Radio.setDataRate(RF24_2MBPS);
	Radio.setPALevel(Power);	// set power to high for better range
	delay(5);
	Network.begin(COMM_CHANNEL, Addr);
	Radio.printDetails();
}


#ifdef ARDUINO

const int RGBStrip::UPDATE_INTERVAL = 500;

const int RGBStrip::PIN_RED_N = 0;
const int RGBStrip::PIN_GREEN_N = 1;
const int RGBStrip::PIN_BLUE_N = 2;

RGBStrip::RGBStrip(const int& pinR, const int& pinG, const int& pinB):
		modeBlink(false),
		blinkPinN(0),
		iteration(0) {
	pins[0] = pinR;
	pins[1] = pinG;
	pins[2] = pinB;
	reset();
}

void RGBStrip::update() {
	if (iteration++ % UPDATE_INTERVAL != 0) { return; }

	if (modeBlink) {
		pinVals[blinkPinN]++;
		if (pinVals[blinkPinN] > 255) {
			reset(false, true);
			blinkPinN++;
			blinkPinN %= 3;
		}
	}
}

void RGBStrip::blink() {
	reset();
	modeBlink = true;
}

void RGBStrip::reset(const bool& resetModes, const bool& resetPins) {
	if (resetModes) {
		modeBlink = false;
	}

	if (resetPins) {
		for (int pinN = 0; pinN < 3; pinN++) {
			setColor(pinN, 0, false);
		}
	}
}

int RGBStrip::getColor(const int& colorN) const {
	return pinVals[colorN];
}

void RGBStrip::setColor(const int& colorN, const int& val, const bool& resetModes) {
	if (resetModes) {
		reset(true, false);
	}

	pinVals[colorN] = val;
	analogWrite(pins[colorN], pinVals[colorN]);
}

void RGBStrip::hsl2rgb(const float& _h, const float& _s, const float& _l,
		int& r, int& g, int& b) {
	float h = fmod(h,360); // cycle H around to 0-360 degrees
	h = 3.14159*h/(float)180; // Convert to radians.
	float s = s>0?(s<1?s:1):0; // clamp S and I to interval [0,1]
	float l = l>0?(l<1?l:1):0;

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

#endif
