#include "protocol.h"

using namespace TRadioProtocol;

#ifndef ARDUINO
void ParsePushPayload(const TMem& Payload, int& ValId, int& Val) {
#else
void ParsePushPayload(const char* Payload,
			int& ValId, int& Val) {
#endif

	ValId = Payload[0];
	Val = (int(Payload[1]) << 24) +
		  (int(Payload[2]) << 16) +
		  (int(Payload[3]) << 8) +
		  int(Payload[4]);
}

#ifndef ARDUINO
void GenGetPayload(const int& ValId, TMem& Payload) {
	if (Payload.Length() != PAYLOAD_LEN) { Payload.Gen(PAYLOAD_LEN); }
#else
void GenGetPayload(const int& ValId, char* Payload) {
#endif

	Payload[0] = (char) ValId;
	Payload[1] = 0xFF;
	Payload[2] = 0xFF;
	Payload[3] = 0xFF;
	Payload[4] = 0xFF;
	Payload[5] = 0xFF;
	Payload[6] = 0xFF;
	Payload[7] = 0xFF;
}

#ifndef ARDUINO
void GenSetPayload(const int& ValId, const int& Val, TMem& Payload) {
	if (Payload.Length() != PAYLOAD_LEN) { Payload.Gen(PAYLOAD_LEN); }
#else
void GenSetPayload(const int& ValId, const int& Val,
			char* Payload) {
#endif
	Payload[0] = (char) ValId;
	Payload[1] = char((Val >> 24) & 0xFF);
	Payload[2] = char((Val >> 16) & 0xFF);
	Payload[3] = char((Val >> 8) & 0xFF);
	Payload[4] = char(Val & 0xFF);
	Payload[5] = 0xFF;
	Payload[6] = 0xFF;
	Payload[7] = 0xFF;
}
