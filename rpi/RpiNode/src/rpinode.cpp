#include "rpinode.h"

void TRPiUtil::BusyWait(const uint32_t& Millis) {
	// Set delay time period.
	struct timeval deltatime;
	deltatime.tv_sec = Millis / 1000;
	deltatime.tv_usec = (Millis % 1000) * 1000;
	struct timeval walltime;
	// Get current time and add delay to find end time.
	gettimeofday(&walltime, NULL);
	struct timeval endtime;
	timeradd(&walltime, &deltatime, &endtime);
	// Tight loop to waste time (and CPU) until enough time as elapsed.
	while (timercmp(&walltime, &endtime, <)) {
		gettimeofday(&walltime, NULL);
	}
}

void TRPiUtil::Sleep(const uint32& Millis) {
	struct timespec sleep;
	sleep.tv_sec = Millis / 1000;
	sleep.tv_nsec = (Millis % 1000) * 1000000L;
	while (clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep, &sleep) && errno == EINTR);
}

/////////////////////////////////////////
// DHT11 - Digital temperature and humidity sensor
const uint64 TDHT11TempHumSensor::MIN_SAMPLING_PERIOD = 2000;
const int TDHT11TempHumSensor::DHT_PULSES = 41;

void TDHT11TempHumSensor::Init(v8::Handle<v8::Object> Exports) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(Isolate, TNodeJsUtil::_NewJs<TDHT11TempHumSensor>);
	tpl->SetClassName(v8::String::NewFromUtf8(Isolate, GetClassId().CStr()));

	// ObjectWrap uses the first internal field to store the wrapped pointer.
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	// Add all methods, getters and setters here.
	NODE_SET_PROTOTYPE_METHOD(tpl, "init", _init);
	NODE_SET_PROTOTYPE_METHOD(tpl, "read", _read);

	Exports->Set(v8::String::NewFromUtf8(Isolate, GetClassId().CStr()), tpl->GetFunction());
}

TDHT11TempHumSensor* TDHT11TempHumSensor::NewFromArgs(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	const int Pin = TNodeJsUtil::GetArgInt32(Args, 0);
	return new TDHT11TempHumSensor(Pin);
}

TDHT11TempHumSensor::TDHT11TempHumSensor(const int& _Pin):
		MmioGpio(nullptr),
		Pin(_Pin),
		Temp(0),
		Hum(0),
		PrevReadTm(0) {}

TDHT11TempHumSensor::~TDHT11TempHumSensor() {
	// TODO release the memory map
}

void TDHT11TempHumSensor::Init() {
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

void TDHT11TempHumSensor::SetLow() {
	*(MmioGpio+10) = 1 << Pin;
}

void TDHT11TempHumSensor::SetHigh() {
	*(MmioGpio+7) = 1 << Pin;
}

void TDHT11TempHumSensor::SetMaxPriority() {
	struct sched_param sched;
	memset(&sched, 0, sizeof(sched));
	// Use FIFO scheduler with highest priority for the lowest chance of the kernel context switching.
	sched.sched_priority = sched_get_priority_max(SCHED_FIFO);
	sched_setscheduler(0, SCHED_FIFO, &sched);
}

void TDHT11TempHumSensor::SetDefaultPriority() {
	struct sched_param sched;
	memset(&sched, 0, sizeof(sched));
	// Go back to default scheduler with default 0 priority.
	sched.sched_priority = 0;
	sched_setscheduler(0, SCHED_OTHER, &sched);
}

uint32_t TDHT11TempHumSensor::Input() {
	return *(MmioGpio+13) & (1 << Pin);
}

void TDHT11TempHumSensor::SetInput() {
	// Set GPIO register to 000 for specified GPIO number.
	*(MmioGpio+((Pin)/10)) &= ~(7<<(((Pin)%10)*3));
}

void TDHT11TempHumSensor::SetOutput() {
	// First set to 000 using input function.
	SetInput();
	// Next set bit 0 to 1 to set output.
	*(MmioGpio+((Pin)/10)) |=  (1<<(((Pin)%10)*3));
}

void TDHT11TempHumSensor::ReadSensor() {
	if (TTm::GetCurUniMSecs() - PrevReadTm < MIN_SAMPLING_PERIOD) { return; }

	// Validate humidity and temperature arguments and set them to zero.
//	Temp = 0.0f;
//	Hum = 0.0f;

	// Store the count that each DHT bit pulse is low and high.
	// Make sure array is initialized to start at zero.
	int pulseCounts[DHT_PULSES*2] = {0};

	SetOutput();
	SetMaxPriority();

	// Set pin high for ~500 milliseconds.
	SetHigh();
	TRPiUtil::Sleep(500);

	// The next calls are timing critical and care should be taken
	// to ensure no unnecssary work is done below.

	// Set pin low for ~20 milliseconds.
	SetLow();//pi_2_mmio_set_low(pin);
	TRPiUtil::BusyWait(20);

	// Set pin at input.
	SetInput();
	// Need a very short delay before reading pins or else value is sometimes still low.
	for (volatile int i = 0; i < 50; ++i) {}

	// Wait for DHT to pull pin low.
	uint32_t count = 0;
	while (Input()) {
		if (++count >= DHT_MAXCOUNT) {
			// Timeout waiting for response.
			SetDefaultPriority();
			throw TExcept::New("DHT11 timed out!");
		}
	}

	// Record pulse widths for the expected result bits.
	for (int i=0; i < DHT_PULSES*2; i+=2) {
		// Count how long pin is low and store in pulseCounts[i]
		while (!Input()) {
			if (++pulseCounts[i] >= DHT_MAXCOUNT) {
				// Timeout waiting for response.
				SetDefaultPriority();
				throw TExcept::New("DHT11 timed out!");
			}
		}
		// Count how long pin is high and store in pulseCounts[i+1]
		while (Input()) {
			if (++pulseCounts[i+1] >= DHT_MAXCOUNT) {
				// Timeout waiting for response.
				SetDefaultPriority();
				throw TExcept::New("DHT11 timed out!");
			}
		}
	}

	// Done with timing critical code, now interpret the results.

	// Drop back to normal priority.
	SetDefaultPriority();

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
	Temp = (float) data[2];
	Hum = (float) data[0];

	PrevReadTm = TTm::GetCurUniMSecs();
}

void TDHT11TempHumSensor::init(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	TDHT11TempHumSensor* JsSensor = ObjectWrap::Unwrap<TDHT11TempHumSensor>(Args.Holder());
	JsSensor->Init();

	Args.GetReturnValue().Set(v8::Undefined(Isolate));
}

void TDHT11TempHumSensor::read(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	TDHT11TempHumSensor* JsSensor = ObjectWrap::Unwrap<TDHT11TempHumSensor>(Args.Holder());

	JsSensor->ReadSensor();

	PJsonVal RetVal = TJsonVal::NewObj();
	RetVal->AddToObj("temperature", JsSensor->GetTemp());
	RetVal->AddToObj("humidity", JsSensor->GetHum());

	Args.GetReturnValue().Set(TNodeJsUtil::ParseJson(Isolate, RetVal));
}

//////////////////////////////////////////////
// module initialization
void Init(v8::Handle<v8::Object> Exports) {
	TDHT11TempHumSensor::Init(Exports);
}

NODE_MODULE(rpinode, Init);
