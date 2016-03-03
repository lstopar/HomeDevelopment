#ifndef PRI_PROTOCOL_H_
#define PRI_PROTOCOL_H_

#include <stdint.h>

#ifdef ARDUINO

#include "Arduino.h"

#include "RF24.h"
#include "RF24Network.h"

#else

#include "base.h"
#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>

#endif

const uint16_t ADDRESS_RPI = 00;
const uint16_t ADDRESS_ARDUINO_SOFA = 01;
const uint16_t ADDRESS_ARDUINO_PIR = 04;

class TRadioValue {
public:
	static const int BYTES;

private:
	char ValId;
	int Val;

public:
	const char& GetValId() const { return ValId; }
	const int& GetValInt() const { return Val; }
	bool GetValBool() const { return Val == 1; }

	void SetValId(const char& _ValId) { ValId = _ValId; }
	void SetVal(const bool& BoolVal);
	void SetVal(const int& IntVal);

	void WriteToBuff(char* Buff) const;
	void ReadFromBuff(const char* Buff);
};

const char VAL_ID_ALL = 0xFF;

const unsigned char COMM_CHANNEL = 0x4C;
const int PAYLOAD_LEN = MAX_FRAME_SIZE - sizeof(RF24NetworkHeader);
const int VALS_PER_PAYLOAD = (PAYLOAD_LEN - 1) / TRadioValue::BYTES;

const unsigned char REQUEST_GET = 65;
const unsigned char REQUEST_SET = 66;
const unsigned char REQUEST_PUSH = 67;
const unsigned char REQUEST_PING = 't';				// 116
const unsigned char REQUEST_CHILD_CONFIG = 'k';		//107

class TRadioProtocol {
public:
	static bool IsValidType(const unsigned char& Type);
	static bool HasPayload(const unsigned char& Type);

#ifndef ARDUINO
	static void ParseGetPayload(const TMem& Payload, TChV& ValIdV);
	static void ParseSetPayload(const TMem& Payload, TVec<TRadioValue>& ValV);
	static void ParsePushPayload(const TMem& Payload, TVec<TRadioValue>& ValV) { ParseSetPayload(Payload, ValV); }
	static void GenGetPayload(const TChV& ValIdV, TMem& Payload);
	static void GenSetPayload(const TVec<TRadioValue>& ValV, TMem& Payload);
	static void GenPushPayload(const TVec<TRadioValue>& ValV, TMem& Payload) { GenSetPayload(ValV, Payload); }
#else
	static int parseGetPayload(const char* Payload, char* buff);
	static int parseSetPayload(const char* Payload, TRadioValue* vals);
	static int parsePushPayload(const char* Payload, TRadioValue* vals) { return parseSetPayload(Payload, vals); }
	static void genGetPayload(const int* ValIdV, const int& ValIdVLen, char* Payload);
	static void genSetPayload(const TRadioValue* ValV, const int& Vals, char* Payload);
	static void genPushPayload(const TRadioValue* ValV, const int& Vals, char* Payload) { genSetPayload(ValV, Vals, Payload); }
#endif

	static void InitRadio(RF24& Radio, RF24Network& Network, const uint16_t& Addr,
			const rf24_pa_dbm_e& Power=RF24_PA_HIGH);
};

#ifdef ARDUINO

class TRgbStrip {
private:
	static const int UPDATE_INTERVAL;

	static const int PIN_RED_N;
	static const int PIN_GREEN_N;
	static const int PIN_BLUE_N;

	enum TRgbMode {
		rmDefault,
		rmBlink,
		rmCycleHsv
	};

	int pins[3];
	int pinVals[3];

	TRgbMode mode;

	int blinkPinN;
	int currHue;

	int iteration;

public:
	TRgbStrip(const int& pinR, const int& pinG, const int& pinB);

	void update();

	int getRed() const { return pinVals[PIN_RED_N]; }
	int getGreen() const { return pinVals[PIN_GREEN_N]; }
	int getBlue() const { return pinVals[PIN_BLUE_N]; }

	void setRed(const int& val, const bool& resetModes=true) { setColor(PIN_RED_N, val, resetModes); }
	void setGreen(const int& val, const bool& resetModes=true) { setColor(PIN_GREEN_N, val, resetModes); }
	void setBlue(const int& val, const bool& resetModes=true) { setColor(PIN_BLUE_N, val, resetModes); }

	void blink();
	void cycleHsv();

	bool isBlinking() const { return mode == TRgbMode::rmBlink; }
	bool isCyclingHsv() const { return mode == TRgbMode::rmCycleHsv; }

	void reset(const bool& resetModes=true, const bool& writePins=true);

private:
	int getColor(const int& colorN) const;
	void setColor(const int& colorN, const int& val, const bool& resetModes=true);

	static void hsl2rgb(const float& h, const float& s, const float& l,
			int& r, int& g, int& b);
};

class TManualDimmer {
private:
	static const int THRESHOLD = 5;

	const int vOutPin;
	const int readPin;
	const int pwmPin;

	const int mnVal;
	const int mxVal;

	int currVal;

public:
	TManualDimmer(const int& vOutPin, const int& readPin, const int& pwmPin,
			const int& mnVal=100, const int& mxVal=1000);

	void init();
	void update();
	int getVal() const { return currVal; }

private:
	int readInput() const;
};

///////////////////////////////////////
// Switch which can be toggled either by clicking
// or programatically
class TManualSwitch {
private:
	const int vOutPin;
	const int readPin;
	const int outputPin;

	bool inputOn;
	bool outputOn;

	void (*onStateChanged)(const bool&);

public:
	TManualSwitch(const int& vOutPin, const int& readPin, const int& switchPin);

	void init();
	void update();

	bool isOn() const { return outputOn; }

	void on();
	void off();
	void toggle();

	void setCallback(void (*_onStateChanged)(const bool&)) { onStateChanged = _onStateChanged; }

private:
	void setOutput(const bool& on);
	bool readSwitch();
};


///////////////////////////////////////
// Digital PIR sensor
class TDigitalPir {
protected:
	const int readPin;

private:
	bool motionDetected;

	void (*onStateChanged)(const bool&);

public:
	TDigitalPir(const int& readPin);
	virtual ~TDigitalPir() {}

	void init();
	void update();

	bool isOn() const { return motionDetected; }

	void setCallback(void (*_onStateChanged)(const bool&)) { onStateChanged = _onStateChanged; }

protected:
	virtual bool readInput();
};

///////////////////////////////////////
// Analog PIR sensor
class TAnalogPir: public TDigitalPir {
private:
	const int threshold;
	const uint64_t onTime;

	uint64_t lastOnTime;

public:
	TAnalogPir(const int& readPin, const uint64_t& onTime, const int& threshold);

protected:
	bool readInput();
};

#endif

#endif
