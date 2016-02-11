#include "rpi.h"

/////////////////////////////////////////
// Utilities
TGpioLayout TRpiUtil::PinLayout = TGpioLayout::glUnset;
TCriticalSection TRpiUtil::CriticalSection;


void TRpiUtil::InitGpio(const TGpioLayout& Layout) {
	TLock Lock(CriticalSection);

	int RetVal;
	switch (Layout) {
	case glUnset:
		throw TExcept::New("Cannot set pin layout to unset!");
		break;
	case glBcmGpio:
		RetVal = wiringPiSetupGpio();
		break;
	case glWiringPi:
		RetVal = wiringPiSetup();
		break;
	default:
		throw TExcept::New("Invalid pin layout type: " + TInt::GetStr(Layout));
	}

	EAssertR(RetVal >= 0, "Failed to initialize GPIO!");
	PinLayout = Layout;
}

void TRpiUtil::SetPinMode(const int& Pin, const int& Mode) {
	EAssert(IsValidMode(Pin, Mode));
	pinMode(Pin, Mode);
}

void TRpiUtil::DigitalWrite(const int& Pin, const bool& High) {
	digitalWrite(Pin, High ? 1 : 0);
}

bool TRpiUtil::DigitalRead(const int& Pin) {
	// TODO check if the pin is an input Pin
	return digitalRead(Pin) == 1;
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

bool TRpiUtil::IsValidMode(const int& Pin, const int& Mode) {
	if (Mode == INPUT || Mode == OUTPUT) { return true; }
	if (Mode == PWM_OUTPUT) {
		return (PinLayout == TGpioLayout::glBcmGpio && Pin == 18) ||
				(PinLayout == TGpioLayout::glWiringPi && Pin == 1);
	}
	if (Mode == GPIO_CLOCK) {
		return (PinLayout == TGpioLayout::glBcmGpio && Pin == 4) ||
				(PinLayout == TGpioLayout::glWiringPi && Pin == 7);
	}
	return false;
}

/////////////////////////////////////////
// DHT11 - Digital temperature and humidity sensor
const uint64 TDHT11Sensor::MIN_SAMPLING_PERIOD = 2000;
const uint64 TDHT11Sensor::SAMPLING_TM = 1000;
const uint32 TDHT11Sensor::DHT_MAXCOUNT = 32000;
const int TDHT11Sensor::DHT_PULSES = 41;

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

		TRpiUtil::SetPinMode(Pin, OUTPUT);
		TRpiUtil::SetMaxPriority();

		// Set pin high for ~500 milliseconds.
		TRpiUtil::DigitalWrite(Pin, true);
		TRpiUtil::Sleep(500);

		// The next calls are timing critical and care should be taken
		// to ensure no unnecssary work is done below.

		// Set pin low for ~20 milliseconds.
		TRpiUtil::DigitalWrite(Pin, false);
		TRpiUtil::BusyWait(20);

		// Set pin at input.
		TRpiUtil::SetPinMode(Pin, INPUT);
		// Need a very short delay before reading pins or else value is sometimes still low.
		for (volatile int i = 0; i < 50; ++i) {}

		// Wait for DHT to pull pin low.
		uint32_t count = 0;
		while (TRpiUtil::DigitalRead(Pin)) {
			if (++count >= DHT_MAXCOUNT) {
				// Timeout waiting for response.
				throw TExcept::New("DHT11 timed out!");
			}
		}

		// Record pulse widths for the expected result bits.
		for (int i = 0; i < DHT_PULSES*2; i += 2) {
			// Count how long pin is low and store in pulseCounts[i]
			while (!TRpiUtil::DigitalRead(Pin)) {
				if (++pulseCounts[i] >= DHT_MAXCOUNT) {
					// Timeout waiting for response.
					throw TExcept::New("DHT11 timed out!");
				}
			}
			// Count how long pin is high and store in pulseCounts[i+1]
			while (TRpiUtil::DigitalRead(Pin)) {
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
constexpr uint32 TYL40Adc::I2C_ADDRESS = 0x48;

const uchar TYL40Adc::ANALOG_OUTPUT = 0x40;
const uchar TYL40Adc::MODE_READ = 0x01;
const uchar TYL40Adc::MODE_WRITE = 0x00;

const uint64 TYL40Adc::PROCESSING_DELAY = 2000;

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

	const int Written = write(FileDesc, Command, CommandLen);
	EAssertR(Written == CommandLen, "Failed to send a command to the YL-40 sensor: written " + TInt::GetStr(Written) +" bytes!");
	usleep(PROCESSING_DELAY);
}

void TYL40Adc::CleanUp() {
	if (FileDesc >= 0) {
		close(FileDesc);
		FileDesc = -1;
	}
}

///////////////////////////////////////////
//// RF24 Radio transmitter
const rf24_pa_dbm_e TRf24Radio::POWER_LEVEL = rf24_pa_dbm_e::RF24_PA_LOW;

void TRf24Radio::TReadThread::Run() {
	Notify->OnNotifyFmt(TNotifyType::ntInfo, "Starting read thread ...");

	uint16 FromNode;
	uchar Type;
	TMem Payload;

	while (true) {
		try {
			Radio->UpdateNetwork();

			while (Radio->Read(FromNode, Type, Payload)) {
				Notify->OnNotifyFmt(TNotifyType::ntInfo, "Received message!");

				if (Radio->Callback == nullptr) { continue; }

				try {
					switch (Type) {
					case REQUEST_PING: {
						Notify->OnNotify(TNotifyType::ntInfo, "Received ping, ignoring ...");
						break;
					} case REQUEST_PUSH: {
						int ValId, Val;
						printf("Payload len: %d\n", Payload.Len());
						TRadioProtocol::ParsePushPayload(Payload, ValId, Val);
						Radio->Callback->OnValue(ValId, Val);
						break;
					} case REQUEST_GET: {
						Notify->OnNotify(TNotifyType::ntWarn, "GET not supported!");
						break;
					} case REQUEST_SET: {
						Notify->OnNotify(TNotifyType::ntWarn, "SET not supported!");
						break;
					} case REQUEST_CHILD_CONFIG: {
						Notify->OnNotify(TNotifyType::ntWarn, "Child woke up and sent configuration request, ignoring ...");
					} default: {
						Notify->OnNotifyFmt(TNotifyType::ntWarn, "Unknown header type: %d", Type);
					}
					}
				} catch (const PExcept& Except) {
					Notify->OnNotifyFmt(TNotifyType::ntErr, "Error when calling read callback: %s", Except->GetMsgStr().CStr());
				}
			}

			delayMicroseconds(500);
		} catch (const PExcept& Except) {
			Notify->OnNotifyFmt(TNotifyType::ntErr, "Error on the read thread: %s", Except->GetMsgStr().CStr());
		}
	}
}

TRf24Radio::TRf24Radio(const uint16& NodeAddr, const uint8& PinCe,
		const uint8_t& PinCs, const uint32& SpiSpeed, const PNotify& _Notify):
		MyAddr(NodeAddr),
		Radio(nullptr),
		Network(nullptr),
		ReadThread(),
		Callback(nullptr),
		Notify(_Notify) {

	Notify->OnNotify(TNotifyType::ntInfo, "Creating radio and network ...");
	Radio = new RF24(PinCe, PinCs, SpiSpeed);
	Network = new RF24Network(*Radio);

	ReadThread = TReadThread(this);
	Notify->OnNotify(TNotifyType::ntInfo, "Radio and network created!");
}

TRf24Radio::~TRf24Radio() {
	delete Network;
	delete Radio;
}


void TRf24Radio::Init() {
	Notify->OnNotify(TNotifyType::ntInfo, "Initializing RF24 radio device ...");

	Radio->begin();
	//============================================
	// testing
	Radio->setAutoAck(true);
	Radio->setRetries(15, 15);
	//============================================
	Radio->setDataRate(RF24_2MBPS);		// IMPORTANT, doesn't work otherwise!!
	Radio->setPALevel(RF24_PA_HIGH);	// set power to high for better range
	delay(5);
	Network->begin(COMM_CHANNEL, MyAddr);
	Radio->printDetails();

	Notify->OnNotify(TNotifyType::ntInfo, "Initialized!");

	if (MyAddr != 00) {
		Notify->OnNotifyFmt(TNotifyType::ntInfo, "Contacting parent node: %d", Network->parent());
		Send(Network->parent(), 'k', TMem());
	}

	ReadThread.Start();
}

bool TRf24Radio::Ping(const uint16& NodeId) {
	Notify->OnNotifyFmt(TNotifyType::ntInfo, "Pipnging node %ud ...", NodeId);
	return Send(NodeId, REQUEST_PING, TMem());
}

bool TRf24Radio::Set(const uint16& NodeId, const int& ValId, const int& Val) {
	TMem Payload;	TRadioProtocol::GenSetPayload(ValId, Val, Payload);
	return Send(NodeId, REQUEST_SET, Payload);
}

bool TRf24Radio::Get(const uint16& NodeId, const int& ValId) {
	TMem Payload;	TRadioProtocol::GenGetPayload(ValId, Payload);
	return Send(NodeId, REQUEST_GET, Payload);
}

bool TRf24Radio::Read(uint16& From, uchar& Type, TMem& Payload) {
	TRpiUtil::SetMaxPriority();
	try {
		TLock Lock(CriticalSection);

		RF24NetworkHeader Header;
		if (Network->available()) {
			Network->peek(Header);

			Notify->OnNotify(TNotifyType::ntInfo, "Received message, reading ...");

			From = Header.from_node;
			Type = Header.type;

			printf("Got request type %d\n", Type);

			if (Type == REQUEST_PING ||
					    REQUEST_CHILD_CONFIG) {
				// the node is just testing
				Network->read(Header, nullptr, 0);
			} else if (Type == REQUEST_GET ||
					   Type == REQUEST_SET ||
					   Type == REQUEST_PUSH) {

				if (Payload.Len() != PAYLOAD_LEN) { Payload.Gen(PAYLOAD_LEN); }

				printf("Payload len: %d\n", Payload.Len());
				Network->read(Header, Payload(), PAYLOAD_LEN);

				printf("Payload len after read: %d\n", Payload.Len());
			} else {
				Notify->OnNotifyFmt(TNotifyType::ntWarn, "Unknown header type %c!", Header.type);
			}

			Notify->OnNotify(TNotifyType::ntInfo, "Message processed!");

			return true;
		}
	} catch (...) {
		Notify->OnNotifyFmt(TNotifyType::ntErr, "Exception while reading!");
	}
	TRpiUtil::SetDefaultPriority();
	return false;
}


bool TRf24Radio::Send(const uint16& NodeAddr, const uchar& Command, const TMem& Buff) {
	TRpiUtil::SetMaxPriority();

	TLock Lock(CriticalSection);
	Notify->OnNotifyFmt(TNotifyType::ntInfo, "Sending message to node %d ...", NodeAddr);

	RF24NetworkHeader Header(NodeAddr, Command);
	bool Success = Network->write(Header, Buff(), Buff.Len());

	TRpiUtil::SetDefaultPriority();

	if (!Success) {
		Notify->OnNotify(TNotifyType::ntWarn, "Failed to send message!");
	}

	return Success;
}

void TRf24Radio::UpdateNetwork() {
	TLock Lock(CriticalSection);
	TRpiUtil::SetMaxPriority();
	Network->update();
	TRpiUtil::SetDefaultPriority();
}
