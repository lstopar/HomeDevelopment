/*
 * rpinode.h
 *
 *  Created on: Jan 18, 2016
 *      Author: lstopar
 */

#ifndef SRC_ENOCEANNODE_H_
#define SRC_ENOCEANNODE_H_

#include <node.h>
#include <node_object_wrap.h>

#include <eoLink.h>
#include <eoEEP_D201xx.h>

#include "nodeutil.h"
#include "enocean.h"

class TNodeJsEoDevice: public node::ObjectWrap {
protected:
	const uint32 DeviceId;
	TEoGateway* Gateway;
public:
	TNodeJsEoDevice(const uint32& _DeviceId, TEoGateway* _Gateway):
		DeviceId(_DeviceId),
		Gateway(_Gateway) {}

	virtual ~TNodeJsEoDevice() {}
	virtual void OnMsg(const eoMessage& Msg) = 0;

protected:
	TEoGateway* GetGateway() { return Gateway; }
	eoDevice* GetDevice() const;
};

/////////////////////////////////////////////
// EnOcean D2-01 device
class TNodeJsD201Device: public TNodeJsEoDevice {
	friend class TNodeJsUtil;
public:
	static void Init(v8::Handle<v8::Object> Exports);
	static const TStr GetClassId() { return "D201Device"; };

private:
	static v8::Persistent<v8::Function> Constructor;

	// callbacks
	v8::Persistent<v8::Function> StatusCb;

public:
	TNodeJsD201Device(const uint32_t& DeviceId, TEoGateway* Gateway):
		TNodeJsEoDevice(DeviceId, Gateway),
		StatusCb() {}
	~TNodeJsD201Device();

	static TNodeJsD201Device* New(const uint16& DeviceId, TEoGateway* Gateway)
		{ return new TNodeJsD201Device(DeviceId, Gateway); }

private:
	JsDeclareProperty(id);
	JsDeclareProperty(type);
	JsDeclareFunction(setOutput);
	JsDeclareFunction(readStatus);

	JsDeclareFunction(on);

public:
	void OnMsg(const eoMessage& Msg);

private:
	void OnStatus(const uint8& Channel, const uint8& Value);
};

/////////////////////////////////////////////
// EnOcean Gateway
class TNodeJsEoGateway: public node::ObjectWrap, public TEoGateway::TEoGatewayCallback {
	friend class TNodeJsUtil;
public:
	static void Init(v8::Handle<v8::Object> Exports);
	static const TStr GetClassId() { return "Gateway"; };

private:
	static TNodeJsEoGateway* NewFromArgs(const v8::FunctionCallbackInfo<v8::Value>& Args);

	TEoGateway Gateway;

	v8::Persistent<v8::Object> DeviceMap;
	v8::Persistent<v8::Function> OnDeviceCallback;

	PNotify Notify;

	TNodeJsEoGateway(const TStr& SerialPort, const TStr& StorageFNm,
			const PNotify& Notify);
	~TNodeJsEoGateway();

private:	// JS functions
	JsDeclareFunction(init);
	JsDeclareFunction(startLearningMode);

	JsDeclareFunction(on);

public:
	void OnDeviceConnected(const uint32& DeviceId);
	void OnMsg(const uint32& DeviceId, const eoMessage& Msg);

private:
	void AddDevice(const uint32& DeviceId, v8::Local<v8::Object>& JsDevice);
	void OnMsgMainThread(const uint32& DeviceId, const eoMessage& Msg);

	TNodeJsEoDevice* GetDevice(const uint32& DeviceId) const;

	class TOnDeviceConnectedTask: public TMainThreadTask {
	private:
		TNodeJsEoGateway* JsGateway;
		const uint32 DeviceId;
	public:
		TOnDeviceConnectedTask(TNodeJsEoGateway* JsGateway, const uint32& DeviceId);
		void Run();
	};

	class TOnMsgTask: public TMainThreadTask {
	private:
		TNodeJsEoGateway* Gateway;
		const uint32 DeviceId;
		const eoMessage Msg;
	public:
		TOnMsgTask(TNodeJsEoGateway* _Gateway, const uint32& DeviceId,
				const eoMessage& Msg);
		void Run();
	};
};

#endif /* SRC_ENOCEANNODE_H_ */
