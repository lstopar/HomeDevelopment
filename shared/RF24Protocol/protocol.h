#ifndef PRI_PROTOCOL_H_
#define PRI_PROTOCOL_H_

#include <stdint.h>

#ifdef ARDUINO
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

const unsigned char COMM_CHANNEL = 0x4C;
const int PAYLOAD_LEN = 8;

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
	static void ParseGetPayload(const TMem& Payload, int& ValId);
	static void ParseSetPayload(const TMem& Payload, int& ValId, int& Val);
	static void ParsePushPayload(const TMem& Payload, int& ValId, int& Val);
	static void GenGetPayload(const int& ValId, TMem& Payload);
	static void GenSetPayload(const int& ValId, const int& Val, TMem& Payload);
	static void GenPushPayload(const int& ValId, const int& Val, TMem& Payload);
#else
	static void ParseGetPayload(const char* Payload, int& ValId);
	static void ParseSetPayload(const char* Payload, int& ValId, int& Val);
	static void ParsePushPayload(const char* Payload, int& ValId, int& Val);
	static void GenGetPayload(const int& ValId, char* Payload);
	static void GenSetPayload(const int& ValId, const int& Val, char* Payload);
	static void GenPushPayload(const int& ValId, const int& Val, char* Payload);
#endif

	static void InitRadio(RF24& Radio, RF24Network& Network, const uint16_t& Addr);
};

#endif
