/*
 * rpi.h
 *
 *  Created on: Jan 21, 2016
 *      Author: lstopar
 */

#ifndef SRC_RPI_H_
#define SRC_RPI_H_

#include "base.h"
#include "threads.h"

#include <wiringPi.h>
#include <wiringPiI2C.h>

#include "RF24/RF24.h"


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
	static constexpr uint64 MIN_SAMPLING_PERIOD = 2000;
	static constexpr uint64 SAMPLING_TM = 1000;
private:
	static constexpr uint32 DHT_MAXCOUNT = 32000;
	static constexpr int DHT_PULSES = 41;

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
	static constexpr uint32 I2C_ADDRESS = 0x48;

	static constexpr uchar ANALOG_OUTPUT = 0x40;
	static constexpr uchar MODE_READ = 0x01;
	static constexpr uchar MODE_WRITE = 0x00;

	static constexpr uint64 PROCESSING_DELAY = 2000;

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
public:
	class TRf24RadioCallback {
	public:
		virtual void OnMsg(const TMem& Msg) = 0;
	};

private:
	class TReadThread: public TThread {
	private:
		TRf24Radio* Radio;
		PNotify Notify;
	public:
		TReadThread():
			Radio(),
			Notify() {}
		TReadThread(TRf24Radio* _Radio):
			Radio(_Radio),
			Notify(_Radio->Notify) {}

		void Run();
	};

	static constexpr rf24_pa_dbm_e POWER_LEVEL = rf24_pa_dbm_e::RF24_PA_LOW;
	static constexpr uint8 RETRY_DELAY = 15;
	static constexpr uint8 RETRY_COUNT = 15;
	static constexpr uint8 COMM_CHANNEL = 0x4C;
	static constexpr int PAYLOAD_SIZE = 8;

	static constexpr uint64_t PIPES[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

	RF24 Radio;
	TReadThread ReadThread;

	TRf24RadioCallback* Callback;

	TCriticalSection CriticalSection;
	PNotify Notify;

public:
	TRf24Radio(const uint8& PinCe, const uint8_t& PinCs, const uint32& SpiSpeed=BCM2835_SPI_SPEED_8MHZ,
			const PNotify& Notify=TNotify::NullNotify);

	void Init();
	bool Send(const TMem& Buff);
	bool Read(TMem& Msg);

	void SetCallback(TRf24RadioCallback* Cb) { Callback = Cb; }
};


#endif /* SRC_RPI_H_ */
