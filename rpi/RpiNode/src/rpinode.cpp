#include "rpinode.h"

volatile uint32_t* MmioGpio = nullptr;

const int TDHT11TempHumSensor::DHT_PULSES = 41;

void TDHT11TempHumSensor::Init(v8::Handle<v8::Object> Exports) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(Isolate, TNodeJsUtil::_NewJs<TDHT11TempHumSensor>);
	tpl->SetClassName(v8::String::NewFromUtf8(Isolate, GetClassId().CStr()));

	// ObjectWrap uses the first internal field to store the wrapped pointer.
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	// Add all methods, getters and setters here.
	NODE_SET_PROTOTYPE_METHOD(tpl, "read", _read);

	Exports->Set(v8::String::NewFromUtf8(Isolate, GetClassId().CStr()), tpl->GetFunction());
}

TDHT11TempHumSensor* TDHT11TempHumSensor::NewFromArgs(
		const v8::FunctionCallbackInfo<v8::Value>& Args) {

	const int Pin = TNodeJsUtil::GetArgInt32(Args, 0);

	return new TDHT11TempHumSensor(Pin);
}

void TDHT11TempHumSensor::ReadSensor(float& Temp, float& Hum) {
	// Validate humidity and temperature arguments and set them to zero.
	Temp = 0.0f;
	Hum = 0.0f;

	// Initialize GPIO library.
	TRPiUtil::MmioInit();

	// Store the count that each DHT bit pulse is low and high.
	// Make sure array is initialized to start at zero.
	int pulseCounts[DHT_PULSES*2] = {0};



	// Set pin high for ~500 milliseconds.
	TRPiUtil::MmioSetHigh(Pin);
	TRPiUtil::Sleep(500);

	// The next calls are timing critical and care should be taken
	// to ensure no unnecssary work is done below.

	// Set pin low for ~20 milliseconds.
	TRPiUtil::MmioSetLow(Pin);//pi_2_mmio_set_low(pin);
	TRPiUtil::BusyWait(20);

	// Set pin at input.
	TRPiUtil::MmioSetInput(Pin);
	// Need a very short delay before reading pins or else value is sometimes still low.
	for (volatile int i = 0; i < 50; ++i) {}

	// Wait for DHT to pull pin low.
	uint32_t count = 0;
	while (TRPiUtil::MmioInput(Pin)) {
		if (++count >= DHT_MAXCOUNT) {
			// Timeout waiting for response.
			TRPiUtil::SetDefaultPriority();
			throw TExcept::New("DHT11 timed out!");
		}
	}

	// Record pulse widths for the expected result bits.
	for (int i=0; i < DHT_PULSES*2; i+=2) {
		// Count how long pin is low and store in pulseCounts[i]
		while (!TRPiUtil::MmioInput(Pin)) {
			if (++pulseCounts[i] >= DHT_MAXCOUNT) {
				// Timeout waiting for response.
				TRPiUtil::SetDefaultPriority();
				throw TExcept::New("DHT11 timed out!");
			}
		}
		// Count how long pin is high and store in pulseCounts[i+1]
		while (TRPiUtil::MmioInput(Pin)) {
			if (++pulseCounts[i+1] >= DHT_MAXCOUNT) {
				// Timeout waiting for response.
				TRPiUtil::SetDefaultPriority();
				throw TExcept::New("DHT11 timed out!");
			}
		}
	}

	// Done with timing critical code, now interpret the results.

	// Drop back to normal priority.
	TRPiUtil::SetDefaultPriority();

	// Compute the average low pulse width to use as a 50 microsecond reference threshold.
	// Ignore the first two readings because they are a constant 80 microsecond pulse.
	int threshold = 0;
	for (int i=2; i < DHT_PULSES*2; i+=2) {
	  threshold += pulseCounts[i];
	}
	threshold /= DHT_PULSES-1;

	// Interpret each high pulse as a 0 or 1 by comparing it to the 50us reference.
	// If the count is less than 50us it must be a ~28us 0 pulse, and if it's higher
	// then it must be a ~70us 1 pulse.
	uint8_t data[5] = {0};
	for (int i=3; i < DHT_PULSES*2; i+=2) {
	  int index = (i-3)/16;
	  data[index] <<= 1;
	  if (pulseCounts[i] >= threshold) {
	    // One bit for long pulse.
	    data[index] |= 1;
	  }
	  // Else zero bit for short pulse.
	}

	// Useful debug info:
	//printf("Data: 0x%x 0x%x 0x%x 0x%x 0x%x\n", data[0], data[1], data[2], data[3], data[4]);

	// Verify checksum of received data.
	EAssertR(data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF), "Checksum error!");
	// Get humidity and temp for DHT11 sensor.
	Hum = (float) data[0];
	Temp = (float) data[2];
}

void TDHT11TempHumSensor::read(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	TDHT11TempHumSensor* JsSensor = ObjectWrap::Unwrap<TDHT11TempHumSensor>(Args.Holder());

	float Temp, Hum;
	JsSensor->ReadSensor(Temp, Hum);

	PJsonVal RetVal = TJsonVal::NewObj();
	RetVal->AddToObj("temperature", Temp);
	RetVal->AddToObj("humidity", Hum);

	Args.GetReturnValue().Set(TNodeJsUtil::ParseJson(Isolate, RetVal));
}
