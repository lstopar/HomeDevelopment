#include "protocol.h"

static bool TRadioProtocol::IsValidType(const unsigned char& Type) {
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
void TRadioProtocol::ParseGetPayload(const TMem& Payload, int& ValId) {
	EAssertR(Payload.Len() == PAYLOAD_LEN, "ParseGetPayload: invalid payload length: " + TInt::GetStr(Payload.Len()));
#else
void TRadioProtocol::ParseGetPayload(const char* Payload, int& ValId) {
#endif
	ValId = (int) Payload[0];
}


#ifndef ARDUINO
void TRadioProtocol::ParseSetPayload(const TMem& Payload, int& ValId, int& Val) {
	EAssertR(Payload.Len() == PAYLOAD_LEN, "ParseSetPayload: invalid payload length: " + TInt::GetStr(Payload.Len()));
#else
void TRadioProtocol::ParseSetPayload(const char* Payload, int& ValId, int& Val) {
#endif
	ValId = (int) Payload[0];
	Val = int(Payload[1] << 24) |
		  int(Payload[2] << 16) |
		  int(Payload[3] << 8) |
		  int(Payload[4]);
}

#ifndef ARDUINO
void TRadioProtocol::ParsePushPayload(const TMem& Payload, int& ValId, int& Val) {
	EAssertR(Payload.Len() == PAYLOAD_LEN, "ParsePushPayload: invalid payload length: " + TInt::GetStr(Payload.Len()));EAssert(Payload.Len() == PAYLOAD_LEN);
#else
void TRadioProtocol::ParsePushPayload(const char* Payload,
			int& ValId, int& Val) {
#endif
	ParseSetPayload(Payload, ValId, Val);
}

#ifndef ARDUINO
void TRadioProtocol::GenGetPayload(const int& ValId, TMem& Payload) {
	if (Payload.Len() != PAYLOAD_LEN) { Payload.Gen(PAYLOAD_LEN); }
#else
void TRadioProtocol::GenGetPayload(const int& ValId, char* Payload) {
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
void TRadioProtocol::GenSetPayload(const int& ValId, const int& Val, TMem& Payload) {
	if (Payload.Len() != PAYLOAD_LEN) { Payload.Gen(PAYLOAD_LEN); }
#else
void TRadioProtocol::GenSetPayload(const int& ValId, const int& Val,
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

#ifndef ARDUINO
void TRadioProtocol::GenPushPayload(const int& ValId, const int& Val, TMem& Payload) {
	if (Payload.Len() != PAYLOAD_LEN) { Payload.Gen(PAYLOAD_LEN); }
#else
void TRadioProtocol::GenPushPayload(const int& ValId, const int& Val,
		char* Payload) {
#endif
	GenSetPayload(ValId, Val, Payload);
}
