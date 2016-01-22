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
//#include <fcntl.h>
#include <sys/mman.h>

#include <linux/i2c-dev.h>

#define GPIO_BASE_OFFSET 0x200000
#define GPIO_LENGTH 4096
#define DHT_MAXCOUNT 32000

class TRPiUtil {
public:
	static void BusyWait(const uint32_t& Millis);
	static void Sleep(const uint32& millis);
};

/////////////////////////////////////////
// DHT11 - Digital temperature and humidity sensor
//
// Hookup (RPi2):
// VCC: 3.3V
// GND: GND
// DATA: GPIO4
class TDHT11Sensor {
private:
	static const uint64 MIN_SAMPLING_PERIOD;
	static const int DHT_PULSES;

	volatile uint32_t* MmioGpio;
	const int Pin;

	float Temp;
	float Hum;

	TCriticalSection CriticalSection;
	uint64 PrevReadTm;

	PNotify Notify;

public:
	TDHT11Sensor(const int& Pin);
	~TDHT11Sensor();

	void Init();
	void Read();

	const float& GetTemp() const { return Temp; }
	const float& GetHum() const { return Hum; }

private:
	void SetLow();
	void SetHigh();

	void SetMaxPriority();
	void SetDefaultPriority();

	uint32_t Input();

	void SetInput();
	void SetOutput();

	void CleanUp();
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
	TYL40Adc();
	~TYL40Adc();

	void Init();
	int Read(const int& InputN);

	void SetOutput(const int& OutputN, const int& Level);


private:
	void SetInput(const int& InputN);
	void SendCommand(const uchar* Command);
};


#endif /* SRC_RPI_H_ */
