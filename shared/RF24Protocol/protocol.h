#ifndef PRI_PROTOCOL_H_
#define PRI_PROTOCOL_H_

#include <stdint.h>

#ifndef ARDUINO
#include "base.h"
#endif

const uint16_t ADDRESS_RPI = 00;
const uint16_t ADDRESS_ARDUINO_SOFA = 01;

const unsigned char COMM_CHANNEL = 90;
const int PAYLOAD_LEN = 8;

const unsigned char REQUEST_GET = 65;
const unsigned char REQUEST_SET = 66;
const unsigned char REQUEST_PUSH = 67;
const unsigned char REQUEST_PING = 't';				// 116
const unsigned char REQUEST_CHILD_CONFIG = 'k';		//107

class TRadioProtocol {
public:
	static bool HasPayload(const uchar& Type);

#ifndef ARDUINO
	static void ParseGetPayload(const TMem& Payload, int& ValId);
	static void ParseSetPayload(const TMem& Payload, int& ValId, int& Val);
	static void ParsePushPayload(const TMem& Payload, int& ValId, int& Val);
	static void GenGetPayload(const int& ValId, TMem& Payload);
	static void GenSetPayload(const int& ValId, const int& Val, TMem& Payload);
#else
	static void ParseGetPayload(const char* Payload, int& ValId);
	static void ParseSetPayload(const char* Payload, int& ValId, int& Val);
	static void ParsePushPayload(const char* Payload, int& ValId, int& Val);
	static void GenGetPayload(const int& ValId, char* Payload);
	static void GenSetPayload(const int& ValId, const int& Val, char* Payload);
#endif
};

#endif
