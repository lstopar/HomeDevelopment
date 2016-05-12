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
#include "thread.h"

class TEoGateway {
public:
	static const uint32 GATEWAY_ADDR;

	class TEoGatewayCallback {
	public:
		virtual void OnDeviceConnected(const uint32& DeviceId, const uchar& ROrg,
				const uchar& Func, const uchar& Type) = 0;
		virtual void OnMsg(const uint32& DeviceId, const eoMessage& Msg) = 0;
	};

private:
	static const uint64 LEARN_MODE_TIME;

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
	TEoGateway(const TStr& SerialPort, const TStr& StorageFNm,
			const PNotify& Notify);

	const uint32& GetId() const { return GATEWAY_ADDR; }

	void Init();
	void StartLearningMode();
	void Send(const eoMessage& Msg);

	eoDevice* GetDevice(const uint32& DeviceId);

	void SetCallback(TEoGatewayCallback* _Callback) { Callback = _Callback; }

private:
	void Read();

	void OnDeviceConnected(const uint32& DeviceId, const uchar& ROrg,
			const uchar& Func, const uchar& Type) const;
};


#endif /* SRC_ENOCEAN_H_ */
