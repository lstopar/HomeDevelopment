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
class TDHT11TempHumSensor: public node::ObjectWrap {
	friend class TNodeJsUtil;
public:
	static void Init(v8::Handle<v8::Object> Exports);
	static const TStr GetClassId() { return "DHT11"; };

private:
	static TDHT11TempHumSensor* NewFromArgs(const v8::FunctionCallbackInfo<v8::Value>& Args);

	static const uint64 MIN_SAMPLING_PERIOD;
	static const int DHT_PULSES;

	volatile uint32_t* MmioGpio;
	const int Pin;

	float Temp;
	float Hum;

	uint64 PrevReadTm;

	TDHT11TempHumSensor(const int& Pin);
	~TDHT11TempHumSensor();

	void Init();

	void SetLow();
	void SetHigh();

	void SetMaxPriority();
	void SetDefaultPriority();

	uint32_t Input();

	void SetInput();
	void SetOutput();

	void ReadSensor();

	const float& GetTemp() const { return Temp; }
	const float& GetHum() const { return Hum; }

private:	// JS functions
	JsDeclareFunction(init);
	JsDeclareSyncAsync(readSync, read, TReadTask);

	class TReadTask: public TNodeTask {
	private:
		TDHT11TempHumSensor* Sensor;
	public:
		TReadTask(const v8::FunctionCallbackInfo<v8::Value>& Args);

		v8::Handle<v8::Function> GetCallback(const v8::FunctionCallbackInfo<v8::Value>& Args);
		v8::Local<v8::Value> WrapResult();

		void Run();
	};
};


#endif /* SRC_RPINODE_H_ */
