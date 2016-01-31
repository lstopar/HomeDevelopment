#include "rpi.h"

/////////////////////////////////////////
// Utilities
TCriticalSection TRpiUtil::CriticalSection;

void TRpiUtil::InitGpio(const TGpioLayout& PinLayout) {
	TLock Lock(CriticalSection);

	int RetVal;
	switch (PinLayout) {
	case glBcmGpio:
		RetVal = wiringPiSetupGpio();
		break;
	case glWiringPi:
		RetVal = wiringPiSetup();
		break;
	default:
		throw TExcept::New("Invalid pin layout type: " + TInt::GetStr(PinLayout));
	}

	EAssertR(RetVal >= 0, "Failed to initialize GPIO!");
}

void TRpiUtil::SetMaxPriority() {
	struct sched_param sched;
	memset(&sched, 0, sizeof(sched));
	// Use FIFO scheduler with highest priority for the lowest chance of the kernel context switching.
	sched.sched_priority = sched_get_priority_max(SCHED_FIFO);
	sched_setscheduler(0, SCHED_FIFO, &sched);
}

void TRpiUtil::SetDefaultPriority() {
	struct sched_param sched;
	memset(&sched, 0, sizeof(sched));
	// Go back to default scheduler with default 0 priority.
	sched.sched_priority = 0;
	sched_setscheduler(0, SCHED_OTHER, &sched);
}

void TRpiUtil::BusyWait(const uint32_t& Millis) {
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
		gettimeofday(&walltime, nullptr);
	}
}

void TRpiUtil::Sleep(const uint32& Millis) {
	struct timespec sleep;
	sleep.tv_sec = Millis / 1000;
	sleep.tv_nsec = (Millis % 1000) * 1000000L;
	while (clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep, &sleep) && errno == EINTR);
}

/////////////////////////////////////////
// DHT11 - Digital temperature and humidity sensor
TDHT11Sensor::TDHT11Sensor(const int& _Pin, const PNotify& _Notify):
		Pin(_Pin),
		Temp(0),
		Hum(0),
		CriticalSection(),
		PrevReadTm(0),
		Notify(_Notify) {}

void TDHT11Sensor::Read() {
	TLock Lock(CriticalSection);

	Notify->OnNotify(TNotifyType::ntInfo, "Reading DHT11 ...");

	const uint64 Dt = TTm::GetCurUniMSecs() - PrevReadTm;
	if (Dt < MIN_SAMPLING_PERIOD) {
		Notify->OnNotifyFmt(TNotifyType::ntInfo, "Less than 2 seconds since last sensor read, diff: %ld, will keep previous values ...", Dt);
		return;
	}

	try {
		// Store the count that each DHT bit pulse is low and high.
		// Make sure array is initialized to start at zero.
		uint32 pulseCounts[DHT_PULSES*2] = {0};

		pinMode(Pin, OUTPUT);
		TRpiUtil::SetMaxPriority();

		// Set pin high for ~500 milliseconds.
		digitalWrite(Pin, HIGH);
		TRpiUtil::Sleep(500);

		// The next calls are timing critical and care should be taken
		// to ensure no unnecssary work is done below.

		// Set pin low for ~20 milliseconds.
		digitalWrite(Pin, LOW);
		TRpiUtil::BusyWait(20);

		// Set pin at input.
		pinMode(Pin, INPUT);
		// Need a very short delay before reading pins or else value is sometimes still low.
		for (volatile int i = 0; i < 50; ++i) {}

		// Wait for DHT to pull pin low.
		uint32_t count = 0;
		while (digitalRead(Pin)) {
			if (++count >= DHT_MAXCOUNT) {
				// Timeout waiting for response.
				throw TExcept::New("DHT11 timed out!");
			}
		}

		// Record pulse widths for the expected result bits.
		for (int i = 0; i < DHT_PULSES*2; i += 2) {
			// Count how long pin is low and store in pulseCounts[i]
			while (!digitalRead(Pin)) {
				if (++pulseCounts[i] >= DHT_MAXCOUNT) {
					// Timeout waiting for response.
					throw TExcept::New("DHT11 timed out!");
				}
			}
			// Count how long pin is high and store in pulseCounts[i+1]
			while (digitalRead(Pin)) {
				if (++pulseCounts[i+1] >= DHT_MAXCOUNT) {
					// Timeout waiting for response.
					throw TExcept::New("DHT11 timed out!");
				}
			}
		}

		// Done with timing critical code, now interpret the results.

		// Drop back to normal priority.
		TRpiUtil::SetDefaultPriority();

		// Compute the average low pulse width to use as a 50 microsecond reference threshold.
		// Ignore the first two readings because they are a constant 80 microsecond pulse.
		uint32 threshold = 0;
		for (int i=2; i < DHT_PULSES*2; i+=2) {
		  threshold += pulseCounts[i];
		}
		threshold /= DHT_PULSES - 1;

		// Interpret each high pulse as a 0 or 1 by comparing it to the 50us reference.
		// If the count is less than 50us it must be a ~28us 0 pulse, and if it's higher
		// then it must be a ~70us 1 pulse.
		uint8_t Data[5] = {0};
		for (int i=3; i < DHT_PULSES*2; i+=2) {
		  int index = (i-3)/16;
		  Data[index] <<= 1;
		  if (pulseCounts[i] >= threshold) {
			  // One bit for long pulse.
			  Data[index] |= 1;
		  }
		  // Else zero bit for short pulse.
		}

		// Verify checksum of received data.
		EAssertR(Data[4] == ((Data[0] + Data[1] + Data[2] + Data[3]) & 0xFF), "Checksum error!");
		// Get humidity and temp for DHT11 sensor.
		Temp = (float) Data[2];
		Hum = (float) Data[0];

		Notify->OnNotifyFmt(TNotifyType::ntInfo, "Read values temperature: %.3f, humidity: %.3f", Temp, Hum);
	} catch (const PExcept& Except) {
		TRpiUtil::SetDefaultPriority();
		throw Except;
	}
	PrevReadTm = TTm::GetCurUniMSecs();
}


/////////////////////////////////////////
// YL-40 - ADC
TYL40Adc::TYL40Adc(const PNotify& _Notify):
		FileDesc(-1),
		CriticalSection(),
		Notify(_Notify) {}

TYL40Adc::~TYL40Adc() {
	CleanUp();
}

void TYL40Adc::Init() {
	TLock Lock(CriticalSection);

	Notify->OnNotify(TNotifyType::ntInfo, "Initializing YL-40 ...");

	if (FileDesc >= 0) {
		Notify->OnNotify(TNotifyType::ntWarn, "YL-40 already initialized, ignoring ...");
		return;
	}

	try {
		FileDesc = wiringPiI2CSetup(I2C_ADDRESS);
		if (FileDesc < 0) {
			FileDesc = -1;
			throw TExcept::New("Failed to setup I2C device!");
		}

		Notify->OnNotify(TNotifyType::ntInfo, "YL-40 initialized!");
	} catch (const PExcept& Except) {
		CleanUp();
		throw Except;
	}
}

uint TYL40Adc::Read(const int& InputN) {
	TLock Lock(CriticalSection);

	EAssert(FileDesc >= 0);

	Notify->OnNotifyFmt(TNotifyType::ntInfo, "Reading YL-40 input %d ...", InputN);
	SetInput(InputN);

	uchar Val;
	int Read;
	for (int ReadN = 0; ReadN < 2; ReadN++) {
		Read = read(FileDesc, &Val, 1);
		EAssertR(Read == 1, "Failed to read YL-40!");
		usleep(PROCESSING_DELAY);
	}

	return (uint) Val;
}

void TYL40Adc::SetOutput(const int& OutputN, const int& Level) {
	Notify->OnNotifyFmt(TNotifyType::ntInfo, "Reading YL-40 output %d to level %d ...", OutputN, Level);
	EAssertR(0 <= OutputN && OutputN <= 3, "Invalid output channel: " + TInt::GetStr(OutputN));

	const uchar Command[2] = { uchar(ANALOG_OUTPUT | uchar(OutputN)), (uchar) Level };
	SendCommand(Command, 2);
}

void TYL40Adc::SetInput(const int& InputN) {
	Notify->OnNotifyFmt(TNotifyType::ntInfo, "Setting YL-40 input %d ...", InputN);
	EAssertR(0 <= InputN && InputN <= 3, "Invalid input channel: " + TInt::GetStr(InputN));

	uchar Command[2] = { uchar(InputN), 0x01u };
	SendCommand(Command, 2);
}

void TYL40Adc::SendCommand(const uchar* Command, const int& CommandLen) {
	TLock Lock(CriticalSection);
	EAssert(FileDesc >= 0);

	const int Written = write(FileDesc, Command, 2);
	EAssertR(Written == 2, "Failed to send a command to the YL-40 sensor: written " + TInt::GetStr(Written) +" bytes!");
	usleep(PROCESSING_DELAY);
}

void TYL40Adc::CleanUp() {
	if (FileDesc >= 0) {
		close(FileDesc);
		FileDesc = -1;
	}
}
