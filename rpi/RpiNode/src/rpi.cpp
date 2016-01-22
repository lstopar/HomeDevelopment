#include "rpi.h"

/////////////////////////////////////////
// Utilities
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
const uint64 TDHT11Sensor::MIN_SAMPLING_PERIOD = 2000;
const int TDHT11Sensor::DHT_PULSES = 41;

TDHT11Sensor::TDHT11Sensor(const int& _Pin):
		MmioGpio(nullptr),
		Pin(_Pin),
		Temp(0),
		Hum(0),
		CriticalSection(),
		PrevReadTm(0),
		Notify(TNotify::StdNotify) {}

TDHT11Sensor::~TDHT11Sensor() {
	CleanUp();
}

void TDHT11Sensor::Init() {
	try {
		TLock Lock(CriticalSection);

		if (MmioGpio == nullptr) {
			Notify->OnNotify(TNotifyType::ntInfo, "Mapping virtual address space for the DHT11 sensor ...");

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

			Notify->OnNotify(TNotifyType::ntInfo, "Done");
		}
	} catch (const PExcept& Except) {
		Notify->OnNotifyFmt(TNotifyType::ntErr, "Failed to map virtual address space, error: %s!", Except->GetMsgStr().CStr());
		CleanUp();
		throw Except;
	}
}

void TDHT11Sensor::Read() {
	TLock Lock(CriticalSection);

	Notify->OnNotify(TNotifyType::ntInfo, "Reading DHT11 ...");

	const uint64 Dt = TTm::GetCurUniMSecs() - PrevReadTm;
	if (Dt < MIN_SAMPLING_PERIOD) {
		Notify->OnNotifyFmt(TNotifyType::ntInfo, "Less than 2 seconds since last sensor read, diff: %ld, will keep previous values ...", Dt);
		return;
	}

	// Validate humidity and temperature arguments and set them to zero.
	Temp = 0.0f;
	Hum = 0.0f;

	try {
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
				throw TExcept::New("DHT11 timed out!");
			}
		}

		// Record pulse widths for the expected result bits.
		for (int i=0; i < DHT_PULSES*2; i+=2) {
			// Count how long pin is low and store in pulseCounts[i]
			while (!Input()) {
				if (++pulseCounts[i] >= DHT_MAXCOUNT) {
					// Timeout waiting for response.
					throw TExcept::New("DHT11 timed out!");
				}
			}
			// Count how long pin is high and store in pulseCounts[i+1]
			while (Input()) {
				if (++pulseCounts[i+1] >= DHT_MAXCOUNT) {
					// Timeout waiting for response.
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

		Notify->OnNotifyFmt(TNotifyType::ntInfo, "Read values temperature: %.3f, humidity: %.3f", Temp, Hum);
	} catch (const PExcept& Except) {
		SetDefaultPriority();
		throw Except;
	}
	PrevReadTm = TTm::GetCurUniMSecs();
}

void TDHT11Sensor::SetLow() {
	*(MmioGpio+10) = 1 << Pin;
}

void TDHT11Sensor::SetHigh() {
	*(MmioGpio+7) = 1 << Pin;
}

void TDHT11Sensor::SetMaxPriority() {
	struct sched_param sched;
	memset(&sched, 0, sizeof(sched));
	// Use FIFO scheduler with highest priority for the lowest chance of the kernel context switching.
	sched.sched_priority = sched_get_priority_max(SCHED_FIFO);
	sched_setscheduler(0, SCHED_FIFO, &sched);
}

void TDHT11Sensor::SetDefaultPriority() {
	struct sched_param sched;
	memset(&sched, 0, sizeof(sched));
	// Go back to default scheduler with default 0 priority.
	sched.sched_priority = 0;
	sched_setscheduler(0, SCHED_OTHER, &sched);
}

uint32_t TDHT11Sensor::Input() {
	return *(MmioGpio+13) & (1 << Pin);
}

void TDHT11Sensor::SetInput() {
	// Set GPIO register to 000 for specified GPIO number.
	*(MmioGpio+((Pin)/10)) &= ~(7<<(((Pin)%10)*3));
}

void TDHT11Sensor::SetOutput() {
	// First set to 000 using input function.
	SetInput();
	// Next set bit 0 to 1 to set output.
	*(MmioGpio+((Pin)/10)) |=  (1<<(((Pin)%10)*3));
}

void TDHT11Sensor::CleanUp() {
	TLock Lock(CriticalSection);

	if (MmioGpio != nullptr) {
		Notify->OnNotify(TNotifyType::ntInfo, "Unmapping virtual address space for the DHT11 sensor ...");
		int ErrCode = munmap((void*) MmioGpio, GPIO_LENGTH);
		EAssertR(ErrCode == 0, "Failed to unmap virtual address space for the DHT11 sensor!");
		MmioGpio = nullptr;
	}
}

/////////////////////////////////////////
// YL-40 - ADC
TYL40Adc::TYL40Adc():
	FileDesc(-1),
	CriticalSection(),
	Notify(TNotify::StdNotify) {}

TYL40Adc::~TYL40Adc() {
	close(FileDesc);
	FileDesc = -1;
}

void TYL40Adc::Init() {
	Notify->OnNotify(TNotifyType::ntInfo, "Initializing YL-40 ...");

	if (FileDesc >= 0) {
		Notify->OnNotify(TNotifyType::ntWarn, "YL-40 already initialized, ignoring ...");
		return;
	}

	FileDesc = open("/dev/i2c-1", O_RDWR);
	if (FileDesc < 0) {
		FileDesc = -1;
		throw TExcept::New("Failed to open I2C device file!");
	}

	const int Code = ioctl(FileDesc, I2C_SLAVE, I2C_ADDRESS);
	EAssertR(Code == 0, "Error while selecting I2C device!");

	Notify->OnNotify(TNotifyType::ntInfo, "YL-40 initialized!");
}

int TYL40Adc::Read(const int& InputN) {
	TLock Lock(CriticalSection);

	Notify->OnNotifyFmt(TNotifyType::ntInfo, "Reading YL-40 input %d ...", InputN);
	SetInput(InputN);

	uchar Val;
	int r;
	for (int ReadN = 0; ReadN < 2; ReadN++) {
		Notify->OnNotify(TNotifyType::ntInfo, "Reading ...");
		r = read(FileDesc, &Val, 1);
		EAssertR(r == 0, "Failed to read YL-40!");
		printf("0x%02x\n", Val);
		usleep(PROCESSING_DELAY);
	}

	return (int) Val;
}

void TYL40Adc::SetOutput(const int& OutputN, const int& Level) {
	Notify->OnNotifyFmt(TNotifyType::ntInfo, "Reading YL-40 output %d to level %d ...", OutputN, Level);
	EAssertR(0 <= OutputN && OutputN <= 3, "Invalid output channel: " + TInt::GetStr(OutputN));
	const uchar Command[2] = { uchar(ANALOG_OUTPUT | uchar(OutputN)), (uchar) Level };
	SendCommand(Command);
}

void TYL40Adc::SetInput(const int& InputN) {
	Notify->OnNotifyFmt(TNotifyType::ntInfo, "Reading YL-40 input %d ...", InputN);
	EAssertR(0 <= InputN && InputN <= 3, "Invalid input channel: " + TInt::GetStr(InputN));
//	uchar Command[2] = { uchar(InputN), 0x01u };
	uchar Command[2] = { uchar(0x40 | ((InputN + 1) & 0x03)), uchar(TRnd().GetUniDevInt(256)) };
	write(FileDesc, &Command, 2);
	usleep(PROCESSING_DELAY);
//	EAssertR(Written == 2, "Failed to send a command to the YL-40 sensor: written " + TInt::GetStr(Written) +" bytes!");

//	SendCommand(Command);
}

void TYL40Adc::SendCommand(const uchar* Command) {
	TLock Lock(CriticalSection);
	EAssert(FileDesc >= 0);

	const int Written = write(FileDesc, Command, 2);
	EAssertR(Written == 2, "Failed to send a command to the YL-40 sensor: written " + TInt::GetStr(Written) +" bytes!");
	usleep(PROCESSING_DELAY);
}
