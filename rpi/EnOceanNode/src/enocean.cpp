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
	Gateway->Notify->OnNotify(TNotifyType::ntInfo, "Started EnOcean read thread ...");

	uint64 TotalIterN = 0;
	uint64 IterN = 0;
	uint64 PrintTm = TTm::GetCurUniMSecs();

	while (true) {
		try {
			Gateway->Read();

			const uint64 CurrTm = TTm::GetCurUniMSecs();
			if (CurrTm - PrintTm > 11000) {
				TotalIterN += IterN;

				Gateway->Notify->OnNotify(ntInfo, "==========================================");
				Gateway->Notify->OnNotify(ntInfo, "ENOCEAN READ THREAD STATISTICS");
				Gateway->Notify->OnNotifyFmt(ntInfo, "Interval length: %s secs", TFlt::GetStr(double(CurrTm - PrintTm) / 1000, 3).CStr());
				Gateway->Notify->OnNotifyFmt(ntInfo, "Iterations in interval: %s", TUInt64::GetStr(IterN).CStr());
				Gateway->Notify->OnNotifyFmt(ntInfo, "Total iterations: %s", TUInt64::GetStr(TotalIterN).CStr());
				Gateway->Notify->OnNotify(ntInfo, "==========================================");

				IterN = 0;
				PrintTm = CurrTm;
			}

			TSysProc::Sleep(1);

			IterN++;
		} catch (const PExcept& Except) {
			Gateway->Notify->OnNotify(ntErr, "Unknown exception on EnOcean read thread!");
			TFOut FOut("errors.txt", true);
			FOut.PutStr("Unknown error on EnOcean read thread!\n");
		}
	}
}

TEoGateway::TEoGateway(const TStr& _SerialPort, const TStr& _StorageFNm,
		const PNotify& _Notify):
		StorageManager(),
		Gateway(),
		SerialPort(_SerialPort),
		StorageFNm(_StorageFNm),
		ReadThread(this),
		GatewaySection(),
		LearnModeStartTm(0),
		Callback(nullptr),
		Notify(_Notify) {

	Notify->OnNotifyFmt(TNotifyType::ntInfo, "Gateway created!");
}

void TEoGateway::Init() {
	Notify->OnNotifyFmt(TNotifyType::ntInfo, "Initializing and reading storage manager from \"%s\" ...", StorageFNm.CStr());
	// initialize storage and load all the devices
	StorageManager.addObject("Gateway", &Gateway);
	Notify->OnNotify(TNotifyType::ntInfo, "Loading ...");
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
		eoDevice* Device = It->second;
		const uint32& DeviceId = It->second->ID;

		Notify->OnNotifyFmt(ntInfo, "Initializing device %u ...", DeviceId);
		if (Device->GetProfile() == nullptr) {
			Notify->OnNotify(ntWarn, "Device does not have a profile!");
		}

		OnDeviceConnected(DeviceId);
	}

	// start the read thread
	ReadThread.Start();
}

void TEoGateway::StartLearningMode() {
	TLock Lock(GatewaySection);

	Notify->OnNotify(TNotifyType::ntInfo, "Starting learning mode ...");
	EAssertR(!Gateway.LearnMode, "Already in learning mode!");

	LearnModeStartTm = TTm::GetCurUniMSecs();
	Gateway.LearnModeOn();
	Notify->OnNotify(TNotifyType::ntInfo, "Learning mode is ON!");
}

void TEoGateway::Send(const eoMessage& Msg) {
	TLock Lock(GatewaySection);
	Notify->OnNotifyFmt(ntInfo, "Sending message of length %d to EO device ...", Msg.dataLength);
	Gateway.Send(Msg);
}

eoDevice* TEoGateway::GetDevice(const uint32& DeviceId) {
	TLock Lock(GatewaySection);
	return Gateway.deviceManager->Get(DeviceId);
}

void TEoGateway::Read() {
	bool InLearnMode;
	uint16 RecV;

	{
		TLock Lock(GatewaySection);
		if (Gateway.LearnMode && TTm::GetCurUniMSecs() - LearnModeStartTm > LEARN_MODE_TIME) {
			Notify->OnNotify(TNotifyType::ntInfo, "Leaving learn mode!");
			Gateway.LearnModeOff();
			StorageManager.Save(StorageFNm.CStr());
		}

		InLearnMode = Gateway.LearnMode;
	}

	{
		TLock Lock(GatewaySection);
		RecV = Gateway.Receive();
	}

	if (RecV == 0) { return; }

	if (InLearnMode) {
		if (RecV & RECV_TEACHIN) {
			Notify->OnNotifyFmt(TNotifyType::ntInfo, "Received TeachIn ...");

			uint32 DeviceId = TUInt::Mx;
			uint8 Dir;

			{
				TLock Lock(GatewaySection);

				eoDebug::Print(Gateway.telegram);

				eoDevice* Device = Gateway.device;
				eoProfile* Profile = Device->GetProfile();

				Profile->GetValue(E_DIRECTION, Dir);

				if (Dir == UTE_DIRECTION_BIDIRECTIONAL) {	// TODO need to handle uni-directional devices
					Notify->OnNotify(TNotifyType::ntInfo, "Resonse expected ...");

					eoMessage Response(7);
					if (Gateway.TeachInModule->CreateUTEResponse(Gateway.telegram, Response, TEACH_IN_ACCEPTED, UTE_DIRECTION_BIDIRECTIONAL) != EO_OK) {
						Notify->OnNotify(TNotifyType::ntErr, "Failed to generate response!");
						return;
					}

					Response.sourceID = GATEWAY_ADDR;

					if (Gateway.Send(Response) == EO_OK) {
						Notify->OnNotify(TNotifyType::ntInfo, "Response sent!");
						Notify->OnNotifyFmt(TNotifyType::ntInfo, "Learned device: %u, saving ...", Device->ID);
						DeviceId = Device->ID;
					} else {
						Device = nullptr;
						Notify->OnNotify(TNotifyType::ntWarn, "Failed to send response!");
					}
				}
			}

			Notify->OnNotify(ntInfo, "Calling device callback ...");
			if (DeviceId != TUInt::Mx) {
				OnDeviceConnected(DeviceId);
			}
		}
	}
	else if (RecV & RECV_TELEGRAM) {
		uint32 DeviceId = TUInt::Mn;
		eoMessage Msg;

		{
			TLock Lock(GatewaySection);

			Notify->OnNotify(TNotifyType::ntInfo, "Received telegram ...");

			Msg = eoMessage(Gateway.telegram);

			if (RecV & RECV_PROFILE) {
				DeviceId = Gateway.device->ID;
				Notify->OnNotifyFmt(TNotifyType::ntInfo, "Telegram from device %u", DeviceId);
			} else {
				Notify->OnNotify(ntInfo, "Do not know the device!");
			}
		}

		if (DeviceId != TUInt::Mn && Callback != nullptr) {
			Notify->OnNotify(ntInfo, "Calling callback ...");
			Callback->OnMsg(DeviceId, Msg);
		}
	}
}

void TEoGateway::OnDeviceConnected(const uint32& DeviceId) const {
	Notify->OnNotify(ntInfo, "Received device in EnOceam Gateway ...");
	if (Callback != nullptr) {
		Notify->OnNotifyFmt(ntInfo, "Calling callback for device ID %u ...", DeviceId);
		Callback->OnDeviceConnected(DeviceId);
	}
}


