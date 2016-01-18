/*
 * rpinode.h
 *
 *  Created on: Jan 18, 2016
 *      Author: lstopar
 */

#ifndef SRC_RPINODE_H_
#define SRC_RPINODE_H_

#include <node.h>
#include <node_object_wrap.h>

#include <sys/mman.h>

#include "base.h"
#include "nodeutil.h"

#define GPIO_BASE_OFFSET 0x200000
#define GPIO_LENGTH 4096
#define DHT_MAXCOUNT 32000

class TRPiUtil {
private:
	static volatile uint32_t* MmioGpio;
public:
	static void BusyWait(const uint32_t& Millis);
	static void Sleep(const uint32& millis);


	static void MmioInit() {
		if (MmioGpio == nullptr) {
			FILE *FPtr = fopen("/proc/device-tree/soc/ranges", "rb");
			EAssertR(FPtr != nullptr, "Failed to open devices file!");
			fseek(FPtr, 4, SEEK_SET);
			unsigned char Buff[4];
			EAssertR(fread(Buff, 1, sizeof(Buff), FPtr) == sizeof(Buff), "Failed to read from devices file!");

			uint32_t peri_base = Buff[0] << 24 | Buff[1] << 16 | Buff[2] << 8 | Buff[3] << 0;
			uint32_t gpio_base = peri_base + GPIO_BASE_OFFSET;
			fclose(FPtr);

			int FileDesc = open("/dev/mem", O_RDWR | O_SYNC);
			EAssertR(FileDesc != -1, "Error opening /dev/mem. Probably not running as root.");

			// Map GPIO memory to location in process space.
			MmioGpio = (uint32_t*) mmap(nullptr, GPIO_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, FileDesc, gpio_base);
			close(FileDesc);

			if (MmioGpio == MAP_FAILED) {
				// Don't save the result if the memory mapping failed.
				MmioGpio = nullptr;
				throw TExcept::New("mmap failed!");
			}
		}
	}

	static void MmioSetLow(const int& GpioNum) {
		*(MmioGpio+10) = 1 << GpioNum;
	}

	static void MmioSetHigh(const int& GpioNum) {
	  *(MmioGpio+7) = 1 << GpioNum;
	}

	static void MmioSetInput(const int& GpioNum) {
	  // Set GPIO register to 000 for specified GPIO number.
	  *(MmioGpio+((GpioNum)/10)) &= ~(7<<(((GpioNum)%10)*3));
	}

	static uint32_t MmioInput(const int& GpioNum) {
	  return *(MmioGpio+13) & (1 << GpioNum);
	}

	static void SetDefaultPriority() {
	  struct sched_param sched;
	  memset(&sched, 0, sizeof(sched));
	  // Go back to default scheduler with default 0 priority.
	  sched.sched_priority = 0;
	  sched_setscheduler(0, SCHED_OTHER, &sched);
	}
};

/////////////////////////////////////////
// DHT11 - Digital temperature and humidity sensor
//
// Hookup (RPi2):
// VCC: 3.3V
// GND: GND
// DATA: GPIO4
class TDHT11TempHumSensor: public node::ObjectWrap {
	friend class TNodeJsUtil;
public:
	static void Init(v8::Handle<v8::Object> Exports);
	static const TStr GetClassId() { return "DHT11"; };

private:
	static TDHT11TempHumSensor* NewFromArgs(const v8::FunctionCallbackInfo<v8::Value>& Args);

	static const int DHT_PULSES;

	int Pin;

	TDHT11TempHumSensor(const int& Pin);
	~TDHT11TempHumSensor() {}

	void ReadSensor(float& Temp, float& Hum);

private:	// JS functions
	JsDeclareFunction(read);
};


#endif /* SRC_RPINODE_H_ */
