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
#include <sys/mman.h>

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
	void ReadSensor();

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


#endif /* SRC_RPI_H_ */
