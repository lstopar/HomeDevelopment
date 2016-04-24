#include "protocol.h"

const int TRadioValue::BYTES = 2;

void TRadioValue::SetVal(const bool& BoolVal) {
	Val = BoolVal ? 1 : 0;
}

void TRadioValue::SetVal(const int& IntVal) {
	Val = IntVal;
}

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
			Type == REQUEST_PONG ||
			Type == REQUEST_ACK ||
			Type == REQUEST_CHILD_CONFIG ||
			Type == REQUEST_GET ||
			Type == REQUEST_PUSH ||
			Type == REQUEST_SET;
}

bool TRadioProtocol::HasPayload(const unsigned char& Type) {
	return Type != REQUEST_PING &&
			Type != REQUEST_CHILD_CONFIG &&
			Type != REQUEST_PONG &&
			Type != REQUEST_ACK;
}

#ifndef ARDUINO
void TRadioProtocol::ParseGetPayload(const TMem& Payload, TChV& ValIdV) {
	EAssertR(Payload.Len() == PAYLOAD_LEN, "ParseGetPayload: invalid payload length: " + TInt::GetStr(Payload.Len()));
#else
void TRadioProtocol::parseGetPayload(const char* Payload, std::vector<char>& ValIdV) {
#endif
	const int Vals = (int) Payload[0];

#ifndef ARDUINO
	EAssertR(Vals <= VALS_PER_PAYLOAD, "Invalid number of values in payload: " + TInt::GetStr(Vals));
	ValIdV.Gen(Vals);
#else
	ValIdV.resize(Vals);
#endif

	for (int ValN = 0; ValN < Vals; ValN++) {
		ValIdV[ValN] = Payload[ValN+1];
	}
}


#ifndef ARDUINO
void TRadioProtocol::ParseSetPayload(const TMem& Payload, TVec<TRadioValue>& ValV) {
	EAssertR(Payload.Len() == PAYLOAD_LEN, "ParseSetPayload: invalid payload length: " + TInt::GetStr(Payload.Len()));
#else
void TRadioProtocol::parseSetPayload(const char* Payload, std::vector<TRadioValue>& ValV) {
#endif
	const int Vals = (int) Payload[0];

#ifndef ARDUINO
	EAssertR(Vals <= VALS_PER_PAYLOAD, "Invalid number of values in payload: " + TInt::GetStr(Vals));
	ValV.Gen(Vals);
#else
	ValV.resize(Vals);
#endif

	for (int ValN = 0; ValN < Vals; ValN++) {
		TRadioValue& Val = ValV[ValN];
		Val.ReadFromBuff(&Payload[ValN*TRadioValue::BYTES + 1]);
	}
}

#ifndef ARDUINO
void TRadioProtocol::GenGetPayload(const TChV& ValIdV, TMem& Payload) {
	if (Payload.Len() != PAYLOAD_LEN) { Payload.Gen(PAYLOAD_LEN); }
	const int& Vals = ValIdV.Len();
	EAssertR(Vals <= VALS_PER_PAYLOAD, "Tried to send too many values!");
#else
void TRadioProtocol::genGetPayload(const int* ValIdV, const int& Vals,
		char* Payload) {
#endif

	Payload[0] = (char) Vals;
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
void TRadioProtocol::genSetPayload(const std::vector<TRadioValue>& ValV, char* Payload) {
	const int Vals = ValV.size();
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
	Radio.setCRCLength(rf24_crclength_e::RF24_CRC_16);

	delay(5);
	Network.begin(COMM_CHANNEL, Addr);
	Radio.printDetails();
}
