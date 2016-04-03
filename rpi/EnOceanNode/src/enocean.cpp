/*
 * enocean.cpp
 *
 *  Created on: Mar 17, 2016
 *      Author: lstopar
 */

#include "enocean.h"

const uint32 TEoGateway::GATEWAY_ADDR = 3333;
const uint64 TEoGateway::LEARN_MODE_TIME = 30000;

void TEoGateway::TReadThread::Run() {
	while (true) {
		Gateway->Read();
		TSysProc::Sleep(1);
	}
}

void TEoGateway::Init() {
	Notify->OnNotifyFmt(TNotifyType::ntInfo, "Initializing and reading storage manager from \"%s\" ...", StorageFNm.CStr());
	// initialize storage and load all the devices
	StorageManager.addObject("Gateway", &Gateway);
	StorageManager.Load(StorageFNm.CStr());

	Notify->OnNotify(TNotifyType::ntInfo, "Setting functions ...");
	// init teachin module
	eoTeachInModule* TeachInModule = Gateway.TeachInModule;
	TeachInModule->SetRPS(0x02, 0x01);	// profile F6-02-xx Rocker switch, 2 rocker

	Notify->OnNotify(TNotifyType::ntInfo, "Opening connection to serial port ...");
	EAssertR(Gateway.Open(SerialPort.CStr()) == EO_OK, "Failed to open serial port: " + SerialPort);

	// print the version
	CO_RD_VERSION_RESPONSE version;
	if (Gateway.commands.ReadVersion(version) == EO_OK) {
		Notify->OnNotifyFmt(TNotifyType::ntInfo, "%s %i.%i.%i.%i, ID:0x%08X on %s\n",
										   version.appDescription,
										   version.appVersion[0], version.appVersion[1], version.appVersion[2], version.appVersion[3],
										   version.chipID,
										   SerialPort.CStr());
	}

	// execute callback for all the devices
	Notify->OnNotify(TNotifyType::ntInfo, "Initializing loaded devices ...");
	eoDeviceManager& DeviceManager = *Gateway.deviceManager;
	const id_device_map& DeviceMap = DeviceManager.GetDeviceList();
	for (auto It = DeviceMap.begin(); It != DeviceMap.end(); It++) {
//		const uint32 DeviceId = It->first;
		OnDeviceConnected(It->second);
	}

	// start the read thread
	ReadThread.Start();
}

void TEoGateway::StartLearningMode() {
	TLock Lock(GatewaySection);
	LearnModeStartTm = TTm::GetCurUniMSecs();
	Gateway.LearnModeOn();
}

void TEoGateway::Send(const eoMessage& Msg) {
	TLock Lock(GatewaySection);
	Gateway.Send(Msg);
}

eoDevice* TEoGateway::GetDevice(const uint32& DeviceId) {
	TLock Lock(GatewaySection);
	return Gateway.deviceManager->Get(DeviceId);
}

void TEoGateway::Read() {
	TLock Lock(GatewaySection);	// TODO this needs to be smarter

	if (Gateway.LearnMode && TTm::GetCurUniMSecs() - LearnModeStartTm > LEARN_MODE_TIME) {
		printf("Leaving learn mode!\n");
		Gateway.LearnModeOff();
	}

	const uint16& RecV = Gateway.Receive();

	if (RecV == 0) { return; }

	if (Gateway.LearnMode) {
		if (RecV & RECV_TEACHIN) {
			Notify->OnNotifyFmt(TNotifyType::ntInfo, "Received TeachIn ...");

			eoDebug::Print(Gateway.telegram);

			eoDevice* Device = Gateway.device;
			eoProfile* Profile = Device->GetProfile();

			uint8_t Dir;
			Profile->GetValue(E_DIRECTION, Dir);

			if (Dir == UTE_DIRECTION_BIDIRECTIONAL) {	// check if a response is expected
				printf("Response expected ...\n");
				eoMessage Response(7);
				if (Gateway.TeachInModule->CreateUTEResponse(Gateway.telegram, Response, TEACH_IN_ACCEPTED, UTE_DIRECTION_BIDIRECTIONAL) != EO_OK) {
					Notify->OnNotify(TNotifyType::ntErr, "Failed to generate response!");
					return;
				}

				Gateway.telegram.sourceID = GATEWAY_ADDR;

				if (Gateway.Send(Response) == EO_OK) {
					Notify->OnNotify(TNotifyType::ntInfo, "Response sent!");
					Notify->OnNotifyFmt(TNotifyType::ntInfo, "Learned device: %u", Device->ID);
					OnDeviceConnected(Device);
				} else {
					Notify->OnNotify(TNotifyType::ntWarn, "Failed to send response!");
				}
			} else {
				Notify->OnNotifyFmt(TNotifyType::ntInfo, "Learned device: %u", Device->ID);
			}
		}
	} else if (RecV & RECV_TELEGRAM) {
		const eoMessage& Msg = Gateway.telegram;

		eoDebug::Print(Msg);

		if (RecV & RECV_PROFILE) {
			eoDevice* Device = Gateway.device;

			if (Callback != nullptr) {
				Callback->OnMsg(Device->ID, Msg);
			}
		}
	}

	if ((RecV & RECV_TELEGRAM_SEC) > 0) {
		printf("Received secure telegram!\n");
		// TODO
	}
	if ((RecV & RECV_ERROR) > 0) {
		printf("Received error telegram!\n");
		// TODO
	}
	if ((RecV & RECV_DEVICE_ADDED) > 0) {
		printf("Device added!\n");
		// TODO
	}
	if ((RecV & RECV_SECTEACHIN) > 0) {
		printf("Received second teachIN!\n");
		// TODO
	}
	if ((RecV & RECV_REMAN) > 0) {
		printf("Received remote management message!\n");
		// TODO
	}
}

void TEoGateway::OnDeviceConnected(const eoDevice* Device) const {
	if (Callback != nullptr) {
		Callback->OnDeviceConnected(Device);
	}
}


