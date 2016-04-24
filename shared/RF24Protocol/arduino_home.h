#ifndef ARDUINO_HOME_H_
#define ARDUINO_HOME_H_

#include <stdint.h>

#include "protocol.h"
#include "Arduino.h"
#include "StandardCplusplus.h"
#include <vector>
#include <queue>

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
	bool isMotionDetected;

	void (*onStateChanged)(const bool&);

public:
	TDigitalPir(const int& readPin);
	virtual ~TDigitalPir() {}

	void init();
	void update();

	bool isOn() const { return isMotionDetected; }

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

class TRadioMsg {
private:
	uint16_t addr;
	unsigned char type;
	char* payload;
	int len;

public:
	TRadioMsg();
	TRadioMsg(const uint16_t& addr, const unsigned char& type,
			const char* payload=NULL, const int& len=0);
	TRadioMsg(const TRadioMsg& msg);
	~TRadioMsg();

	TRadioMsg& operator =(const TRadioMsg& msg);

	const uint16_t& getAddr() const { return addr; }
	const unsigned char& getType() const { return type; }
	const char* getPayload() const { return payload; }

private:
	void cleanUp();
};

///////////////////////////////////////
// Radio wrapper
class TRf24Wrapper {
private:
	uint16_t address;

	RF24& radio;
	RF24Network& network;

	std::queue<TRadioMsg> receiveQ;

	void (*processGet)(const uint16_t&, const std::vector<char>&);
	void (*processSet)(const uint16_t&, const std::vector<TRadioValue>&);

	char receivePayload[PAYLOAD_LEN];

public:
	TRf24Wrapper(const uint16_t address, RF24& radio, RF24Network& network);

	void init();
	void update();

	bool push(const uint16_t& to, const std::vector<TRadioValue>& values);
	bool push(const uint16_t& to, const TRadioValue& value);

	void setProcessGet(void (*_processGet)(const uint16_t&, const std::vector<char>&)) { processGet = _processGet; }
	void setProcessSet(void (*_processSet)(const uint16_t&, const std::vector<TRadioValue>&)) { processSet = _processSet; }

private:
	bool _send(const uint16_t& to, const unsigned char& type, const char* payload=NULL,
			const int& payloadLen=0);
	void processMsg(const TRadioMsg& msg) const;
};

#endif
