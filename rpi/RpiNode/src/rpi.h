/*
 * rpi.h
 *
 *  Created on: Jan 21, 2016
 *      Author: lstopar
 */

#ifndef SRC_RPI_H_
#define SRC_RPI_H_

#include <wiringPi.h>
#include <wiringPiI2C.h>

#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>

#include "base.h"
#include "threads.h"
#include "protocol.h"

enum TGpioLayout {
	glUnset,
	glWiringPi,
	glBcmGpio
};

class TRpiUtil {
private:
	static TGpioLayout PinLayout;
	static TCriticalSection CriticalSection;
public:
	static void InitGpio(const TGpioLayout& PinLayout=TGpioLayout::glBcmGpio);
	static void SetPinMode(const int& Pin, const int& Mode);
	static void DigitalWrite(const int& Pin, const bool& High);
	static bool DigitalRead(const int& Pin);

	static void SetMaxPriority();
	static void SetDefaultPriority();

	static void BusyWait(const uint32_t& Millis);
	static void Sleep(const uint32& millis);

private:
	static bool IsValidMode(const int& Pin, const int& Mode);
};

/////////////////////////////////////////
// DHT11 - Digital temperature and humidity sensor
//
// Hookup (RPi2):
// VCC: 3.3V
// GND: GND
// DATA: GPIO4
class TDHT11Sensor {
public:
	static const uint64 MIN_SAMPLING_PERIOD;
	static const uint64 SAMPLING_TM;
private:
	static const uint32 DHT_MAXCOUNT;
	static const int DHT_PULSES;

	const int Pin;

	float Temp;
	float Hum;

	TCriticalSection CriticalSection;
	uint64 PrevReadTm;

	PNotify Notify;

public:
	TDHT11Sensor(const int& Pin, const PNotify& Notify);

	void Init() {}
	void Read();

	const float& GetTemp() const { return Temp; }
	const float& GetHum() const { return Hum; }
};

/////////////////////////////////////////
// YL-40 - Analog to digital signal converter + 3 sensors
//
// Hookup (RPi2):
// VCC: 3.3V
// GND: GND
// SDA: GPIO2
// SCL: GPIO3
//
// Reading the sensor from bash (the value that is sampled is returned in the next read):
// # read the temperature (channel 0)
// i2cget -y 1 0x48
// # read channel 1
// i2cset -y 1 0x48 0x01
// i2cget -y 1 0x48
// # analog output (add bit 0x40 to the set command)
// i2cset -y 1 0x48 0x41 0xff
class TYL40Adc {
private:
	static const uint32 I2C_ADDRESS;

	static const uchar ANALOG_OUTPUT;
	static const uchar MODE_READ;
	static const uchar MODE_WRITE;

	static const uint64 PROCESSING_DELAY;

	int FileDesc;
	TCriticalSection CriticalSection;
	PNotify Notify;

public:
	TYL40Adc(const PNotify& Notify);
	~TYL40Adc();

	void Init();
	uint Read(const int& InputN);
	void SetOutput(const int& OutputN, const int& Level);

private:
	void SetInput(const int& InputN);
	void SendCommand(const uchar* Command, const int& CommandLen);

	void CleanUp();
};

///////////////////////////////////////////
//// RF24 Radio transmitter
class TRf24Radio {
	typedef TTriple<TUInt16, TUCh, TMem> TMsgInfo;
	typedef TVec<TMsgInfo> TMsgInfoV;
public:
	class TRf24RadioCallback {
	public:
		virtual void OnPong(const uint16& NodeId) = 0;
		virtual void OnValue(const uint16& NodeId, const char& ValId, const int& Val) = 0;
	};

private:
	class TReadThread: public TThread {
	private:
		TRf24Radio* Radio;
		TVec<TMsgInfo> MsgQ;
		PNotify Notify;
	public:
		TReadThread():
			Radio(),
			MsgQ(),
			Notify() {}
		TReadThread(TRf24Radio* _Radio):
			Radio(_Radio),
			MsgQ(),
			Notify(_Radio->Notify) {}

		void Run();
		void AddToQueue(const TMsgInfo& Msg);
		void AddToQueue(const TMsgInfoV& MsgV);

	private:
		void ProcessMsg(const uint16& FromNode, const uchar& Type, const TMem& Payload) const;
	};

	static const uint64 ACK_TIMEOUT;

	const uint16 MyAddr;
	RF24* Radio;
	RF24Network* Network;
	TReadThread ReadThread;

	const int RetryCount;

	TRf24RadioCallback* Callback;

	TCriticalSection CriticalSection;
	PNotify Notify;

public:
	TRf24Radio(const uint16& NodeAddr, const uint8& PinCe, const uint8_t& PinCs,
			const uint32& SpiSpeed=BCM2835_SPI_SPEED_8MHZ, const PNotify& Notify=TNotify::NullNotify);
	~TRf24Radio();

	void Init();
	bool Ping(const uint16& NodeId);
	bool Pong(const uint16& NodeId);
	bool Set(const uint16& NodeId, const int& ValId, const int& Val);
	bool Set(const uint16& NodeId, const TIntPrV& ValIdValPrV);
	bool Get(const uint16& NodeId, const int& ValId);
	bool GetAll(const uint16& NodeId);

	void SetCallback(TRf24RadioCallback* Cb) { Callback = Cb; }

private:
	bool Send(const uint16& To, const uchar& Command, const TMem& Buff);
	void _Send(RF24NetworkHeader& Header, const TMem& Buff=TMem());

	void UpdateNetwork();
	bool Read(uint16& From, uchar& Type, TMem& Payload);
};


#endif /* SRC_RPI_H_ */
