/*
 * rpinode.h
 *
 *  Created on: Jan 18, 2016
 *      Author: lstopar
 */

#ifndef SRC_RPINODE_H_
#define SRC_RPINODE_H_

#include <node.h>
#include <node_object_wrap.h>

#include <eoLink.h>
#include <eoEEP_D201xx.h>

#include "rpi.h"
#include "enocean.h"
#include "nodeutil.h"


class TNodeJsRpiBase: public node::ObjectWrap {
	friend class TNodeJsUtil;
public:
	static void Init(v8::Handle<v8::Object> Exports);

	JsDeclareFunction(init);
//	JsDeclareFunction(pinMode);
};

/////////////////////////////////////////
// DHT11 - Digital temperature and humidity sensor
//
// Hookup (RPi2):
// VCC: 3.3V
// GND: GND
// DATA: GPIO4
class TNodejsDHT11Sensor: public node::ObjectWrap {
	friend class TNodeJsUtil;
public:
	static void Init(v8::Handle<v8::Object> Exports);
	static const TStr GetClassId() { return "DHT11"; };

private:
	static TNodejsDHT11Sensor* NewFromArgs(const v8::FunctionCallbackInfo<v8::Value>& Args);

	TDHT11Sensor* Sensor;
	const TStr TempReadingId;
	const TStr HumReadingId;

	const uint64 MxSampleTm;

	PNotify Notify;

	TNodejsDHT11Sensor(TDHT11Sensor* Sensor, const TStr& TempReadingId, const TStr& HumReadingId,
			const uint64& MxSampleTm, const PNotify& Notify);
	~TNodejsDHT11Sensor();

private:	// JS functions
	JsDeclareFunction(init);
	JsDeclareSyncAsync(readSync, read, TReadTask);

	class TReadTask: public TNodeTask {
	private:
		TNodejsDHT11Sensor* JsSensor;
	public:
		TReadTask(const v8::FunctionCallbackInfo<v8::Value>& Args);

		v8::Handle<v8::Function> GetCallback(const v8::FunctionCallbackInfo<v8::Value>& Args);
		v8::Local<v8::Value> WrapResult();

		void Run();
	};
};

/////////////////////////////////////////
// YL-40 - Analog to digital signal converter + 3 sensors
class TNodeJsYL40Adc: public node::ObjectWrap {
	friend class TNodeJsUtil;
public:
	static void Init(v8::Handle<v8::Object> Exports);
	static const TStr GetClassId() { return "YL40Adc"; };

private:
	static TNodeJsYL40Adc* NewFromArgs(const v8::FunctionCallbackInfo<v8::Value>& Args);

	TYL40Adc* Adc;
	TIntStrKdV InputNumNmKdV;

	PNotify Notify;

	TNodeJsYL40Adc(TYL40Adc* Adc, const TIntStrKdV& InputNumNmKdV, const PNotify& Notify);
	~TNodeJsYL40Adc();

private:
	JsDeclareFunction(init);
	JsDeclareFunction(setOutput);
	JsDeclareSyncAsync(readSync, read, TReadTask);

	class TReadTask: public TNodeTask {
	private:
		TNodeJsYL40Adc* Adc;
		TIntV ValV;
	public:
		TReadTask(const v8::FunctionCallbackInfo<v8::Value>& Args);

		v8::Handle<v8::Function> GetCallback(const v8::FunctionCallbackInfo<v8::Value>& Args);
		v8::Local<v8::Value> WrapResult();

		void Run();
	};
};

/////////////////////////////////////////
// RF24 - Radio
class TNodeJsRf24Radio: public node::ObjectWrap, public TRf24Radio::TRf24RadioCallback {
	friend class TNodeJsUtil;
public:
	static void Init(v8::Handle<v8::Object> Exports);
	static const TStr GetClassId() { return "Rf24"; };

private:
	static TNodeJsRf24Radio* NewFromArgs(const v8::FunctionCallbackInfo<v8::Value>& Args);

	TNodeJsRf24Radio(const uint16& NodeId, const int& PinCE, const int& PinCSN, const TStrIntH& ValueNmIdH,
			const TStrIntH& ValueNmNodeIdH, const PNotify& Notify);
	~TNodeJsRf24Radio();

	TRf24Radio Radio;

	// structures to convert from JS to cpp IDs
	THash<TStr,TIntPr> ValNmNodeIdValIdPrH;
	THash<TIntPr,TStr> NodeIdValIdPrValNmH;
	THashSet<TUInt16> NodeIdSet;

	TVec<TUInt16> PongQ;
	TVec<TTriple<TUInt16,TCh,TInt>> ValQ;

	v8::Persistent<v8::Function> OnValueCallback;
	v8::Persistent<v8::Function> OnPongCallback;

	TMainThreadHandle* CallbackHandle;
	TCriticalSection CallbackSection;

	PNotify Notify;

private:
	JsDeclareFunction(init);
	JsDeclareFunction(get);
	JsDeclareFunction(getAll);
	JsDeclareFunction(set);
	JsDeclareFunction(ping);

	// callbacks
	JsDeclareFunction(on);

	void ProcessQueues();

public:
	void OnPong(const uint16& NodeId);
	void OnValue(const uint16& NodeId, const char& ValId, const int& Val);

	class TProcessQueuesTask: public TMainThreadTask {
	private:
		TNodeJsRf24Radio* JsRadio;
	public:
		TProcessQueuesTask(TNodeJsRf24Radio* _JsRadio):
			JsRadio(_JsRadio) {}

		void Run();
	};
};

class TNodeJsEoDevice: public node::ObjectWrap {
protected:
	const uint32 DeviceId;
	TEoGateway* Gateway;
	PNotify Notify;
public:
	TNodeJsEoDevice(const uint32& DeviceId, TEoGateway* Gateway, const PNotify& Notify);
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
	TNodeJsD201Device(const uint32& DeviceId, TEoGateway* Gateway, const PNotify& Notify):
		TNodeJsEoDevice(DeviceId, Gateway, Notify),
		StatusCb() {}
	~TNodeJsD201Device();

	static TNodeJsD201Device* New(const uint32& DeviceId, TEoGateway* Gateway, const PNotify& Notify)
		{ return new TNodeJsD201Device(DeviceId, Gateway, Notify); }

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

	TUIntV NewDeviceIdQ;
	TVec<TPair<TUInt, eoMessage>> DeviceIdMsgPrQ;

	TMainThreadHandle* CallbackHandle;
	TCriticalSection CallbackSection;

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
	void ProcessQueues();

	TNodeJsEoDevice* GetDevice(const uint32& DeviceId) const;

	class TProcessQueuesTask: public TMainThreadTask {
	private:
		TNodeJsEoGateway* JsGateway;
	public:
		TProcessQueuesTask(TNodeJsEoGateway* JsGateway);
		void Run();
	};
};

#endif /* SRC_RPINODE_H_ */
