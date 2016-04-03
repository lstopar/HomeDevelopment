/**
 * \addtogroup tutorial
 * @{
 * \file HelloWorld.cpp
 * \brief Hello World : Gateway Receiver with output
 * \author EnOcean GmbH
 *
 * Welcome to the World of eoLink, this Hello World will add an eoLink Gateway and prints out all received telegram to the console.
*/
#ifdef WIN32
#include <windows.h>
#define SER_PORT "\\\\.\\COM43"  //COM43
#else
#define SER_PORT "/dev/ttyUSB0"//! the Serial Port Device
#endif
#include <eoLink.h>

#include <chrono>
#include <stdio.h>

#include <eoEEP_D201xx.h>

#include "base.h"

const uint32_t MY_ID = 3333;

eoGateway Gateway;

const uint64 StartTm = TTm::GetCurUniMSecs();
const uint64 LEARN_MODE_TIME = 30000;
uint64 PrevSendTm = 0;
uint8 RelayVal = 0;

void InitGateway(eoGateway& Gateway) {
	eoTeachInModule* TeachInModule = Gateway.TeachInModule;

	TeachInModule->SetRPS(0x02, 0x01);	// profile F6-02-xx Rocker switch, 2 rocker
}

void SendMsg() {
	const uint32_t DeviceId = 26243606;

	eoMessage msg;
	if (eoEEP_D201xx::CreateSetOutput(MY_ID, DeviceId, 0x00, 0, RelayVal, msg) != EO_OK) {
		printf("Failed to generate message!\n");
		return;
	}

	if (Gateway.Send(msg) == EO_OK) {
		printf("Sent!\n");
	} else {
		printf("Failed to send!\n");
	}

	RelayVal = 1 - RelayVal;
}

int main(int argc, const char* argv[]) {
	uint16_t recv;
	CO_RD_VERSION_RESPONSE version;
	//First a Gateway will be defined

	InitGateway(Gateway);

	//This tries to open an connection to the USB300 or fails otherwise
	printf("Opening Connection to USB300 \n");
	if (Gateway.Open(SER_PORT) != EO_OK) {
		printf("Failed to open USB300\n");
		return 0;
	}
	printf("Hello to the world of eoLink\n");

	if (Gateway.commands.ReadVersion(version) == EO_OK) {
		printf("%s %i.%i.%i.%i, ID:0x%08X on %s\n",
								   version.appDescription,
								   version.appVersion[0], version.appVersion[1], version.appVersion[2], version.appVersion[3],
								   version.chipID,
								   SER_PORT);
	}

	Gateway.LearnModeOn();

	while (true) {
		if (Gateway.LearnMode && TTm::GetCurUniMSecs() - StartTm > LEARN_MODE_TIME) {
			printf("Leaving learn mode!\n");
			Gateway.LearnModeOff();
		}

		if (!Gateway.LearnMode && TSysTm::GetCurUniMSecs() - PrevSendTm > 7000) {
			PrevSendTm = TSysTm::GetCurUniMSecs();
			SendMsg();
		}

		recv = Gateway.Receive();

		if (recv == 0) { continue; }

		if ((recv & RECV_TELEGRAM) > 0) {
			eoDebug::Print(Gateway.telegram);
		}
		if ((recv & RECV_PACKET) > 0) {
			eoDebug::Print(Gateway.packet);
		}
		if ((recv & RECV_MESSAGE) > 0) {
			const eoMessage& Msg = Gateway.message;

			eoDebug::Print(Msg);
		}

		if (Gateway.LearnMode) {
			if ((recv & RECV_TEACHIN) > 0) {
				printf("Received teachin!\n");
				eoDebug::Print(Gateway.telegram);
				eoDevice* Device = Gateway.device;
				eoProfile* Profile = Device->GetProfile();

				uint8_t Dir;
				Profile->GetValue(E_DIRECTION, Dir);

				if (Dir == UTE_DIRECTION_BIDIRECTIONAL) {	// check if a response is expected
					printf("Response expected ...\n");
					eoMessage Response(7);
					if (Gateway.TeachInModule->CreateUTEResponse(Gateway.telegram, Response, TEACH_IN_ACCEPTED, UTE_DIRECTION_BIDIRECTIONAL) != EO_OK) {
						printf("Failed to generate response!\n");
						continue;
					}

					Gateway.telegram.sourceID = 3333;

					if (Gateway.Send(Response) == EO_OK) {
						printf("Response sent!\n");
						printf("Learned device: %u\n", Device->ID);
					} else {
						printf("Failed to send response!\n");
					}
				} else {
					printf("Learned device: %u\n", Device->ID);
				}
			}
		} else if ((recv & RECV_TELEGRAM) > 0) {
			const eoMessage& Msg = Gateway.telegram;

			eoDebug::Print(Msg);

			if ((recv & RECV_PROFILE) > 0) {
				printf("Received profile!\n");
				eoDevice* Device = Gateway.device;
				eoProfile* Profile = Device->GetProfile();

				Profile->Parse(Msg);

				uint8_t cmd;
				if (Profile->GetValue(E_COMMAND, cmd) != EO_OK) { printf("Failed to parse command!\n"); }

				if (cmd == ACTUATOR_STATUS_RESPONSE) {
					uint8_t channel;
					uint8_t value;

					Profile->GetValue(E_IO_CHANNEL, channel);
					Profile->GetValue(F_ON_OFF, value);

					printf("Channel %d is %d\n", channel, value);
				} else {
					printf("Unknown command %u\n", cmd);
				}


//
//				uint8 RockerAVal, RockerBVal;
//				Profile->GetValue(CHANNEL_TYPE::E_ROCKER_A, RockerAVal);
//				Profile->GetValue(CHANNEL_TYPE::E_ROCKER_B, RockerBVal);
//
//				printf("Rocker A: %d, B: %d\n", RockerAVal, RockerBVal);
			}
		}

		if ((recv & RECV_TELEGRAM_SEC) > 0) {
			printf("Received secure telegram!\n");
			// TODO
		}
		if ((recv & RECV_ERROR) > 0) {
			printf("Received error telegram!\n");
			// TODO
		}
		if ((recv & RECV_DEVICE_ADDED) > 0) {
			printf("Device added!\n");
			// TODO
		}
		if ((recv & RECV_SECTEACHIN) > 0) {
			printf("Received second teachIN!\n");
			// TODO
		}
		if ((recv & RECV_REMAN) > 0) {
			printf("Received remote management message!\n");
			// TODO
		}
	}
	return 0;
}
/** @}*/
