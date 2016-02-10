#ifndef SRC_RPI_H_
#define SRC_RPI_H_

namespace TRadioProtocol {

	const uint16_t ADDRESS_RPI = 00;
	const uint16_t ADDRESS_ARDUINO_SOFA = 01;

	const unsigned char COMM_CHANNEL = 90;
	const int PAYLOAD_LEN = 8;

	const unsigned char REQUEST_GET = 65;
	const unsigned char REQUEST_SET = 66;
	const unsigned char REQUEST_PUSH = 67;
	const unsigned char REQUEST_PING = 't';
	const unsigned char REQUEST_CHILD_CONFIG = 'k';

#ifndef ARDUINO
	void ParsePushPayload(const TMem& Payload, int& ValId, int& Val);
	void GenGetPayload(const int& ValId, TMem& Payload);
	void GenSetPayload(const int& ValId, const int& Val, TMem& Payload);
#else
	void ParsePushPayload(const char* Payload, int& ValId, int& Val);
	void GenGetPayload(const int& ValId, char* Payload);
	void GenSetPayload(const int& ValId, const int& Val, char* Payload);
#endif

};

#endif
