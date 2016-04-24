#ifndef PRI_PROTOCOL_H_
#define PRI_PROTOCOL_H_

#include <stdint.h>

#ifdef ARDUINO

#include "Arduino.h"

#include "RF24.h"
#include "RF24Network.h"

#include "StandardCplusplus.h"
#include <vector>

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

const int RETRY_COUNT = 3;
const uint64_t ACK_TIMEOUT = 80;

const unsigned char COMM_CHANNEL = 0x4C;
const int PAYLOAD_LEN = MAX_FRAME_SIZE - sizeof(RF24NetworkHeader);
const int VALS_PER_PAYLOAD = (PAYLOAD_LEN - 1) / TRadioValue::BYTES;

const unsigned char REQUEST_GET = 65;
const unsigned char REQUEST_SET = 66;
const unsigned char REQUEST_PUSH = 67;
const unsigned char REQUEST_PONG = 68;
const unsigned char REQUEST_ACK = 69;
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
	static void parseGetPayload(const char* Payload, std::vector<char>& ValIdV);
	static void parseSetPayload(const char* Payload, std::vector<TRadioValue>& ValV);
	static void parsePushPayload(const char* Payload, std::vector<TRadioValue>& ValV) { parseSetPayload(Payload, ValV); }
	static void genGetPayload(const int* ValIdV, const int& ValIdVLen, char* Payload);
	static void genSetPayload(const std::vector<TRadioValue>& values, char* Payload);
	static void genPushPayload(const std::vector<TRadioValue>& values, char* payload) { genSetPayload(values, payload); }
#endif

	static void InitRadio(RF24& Radio, RF24Network& Network, const uint16_t& Addr,
			const rf24_pa_dbm_e& Power=RF24_PA_HIGH);
};

#endif
