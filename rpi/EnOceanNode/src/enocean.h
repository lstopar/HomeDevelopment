/*
 * enocean.h
 *
 *  Created on: Mar 17, 2016
 *      Author: lstopar
 */

#ifndef SRC_ENOCEAN_H_
#define SRC_ENOCEAN_H_

#include <eoLink.h>
#include "base.h"
#include "threads.h"

class TEoGateway {
public:
	static constexpr uint32 GATEWAY_ADDR = 3333;

	class TEoGatewayCallback {
	public:
		virtual void OnDeviceConnected(const eoDevice* Device) = 0;
		virtual void OnMsg(const uint32& DeviceId, const eoMessage& Msg) = 0;
	};

private:
	static constexpr uint64 LEARN_MODE_TIME = 30000;

	class TReadThread: public TThread {
	private:
		TEoGateway* Gateway;
	public:
		TReadThread(): Gateway(nullptr) {}
		TReadThread(TEoGateway* _Gateway): Gateway(_Gateway) {}
		void Run();
	};

	eoStorageManager StorageManager;
	eoGateway Gateway;

	TStr SerialPort;
	TStr StorageFNm;

	TReadThread ReadThread;
	TCriticalSection GatewaySection;

	uint64 LearnModeStartTm;

	TEoGatewayCallback* Callback;

	PNotify Notify;

public:
	TEoGateway(const TStr& _SerialPort, const TStr& _StorageFNm, const PNotify& _Notify):
		StorageManager(),
		Gateway(),
		SerialPort(_SerialPort),
		StorageFNm(_StorageFNm),
		ReadThread(),
		GatewaySection(),
		LearnModeStartTm(0),
		Callback(nullptr),
		Notify(_Notify) {

		ReadThread = TReadThread(this);
	}

	const uint32& GetId() const { return GATEWAY_ADDR; }

	void Init();
	void StartLearningMode();
	void Send(const eoMessage& Msg);

	eoDevice* GetDevice(const uint32& DeviceId);

	void SetCallback(TEoGatewayCallback* _Callback) { Callback = _Callback; }

private:
	void Read();

	void OnDeviceConnected(const eoDevice* Device) const;
};


#endif /* SRC_ENOCEAN_H_ */
