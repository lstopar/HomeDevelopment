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

#include "rpi.h"
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
//	TStrIntH ValueNmIdH;
//	TStrIntH ValueNmNodeIdH;
	THash<TStr, TIntPr> ValNmNodeIdValIdPrH;
	THash<TIntPr, TStr> NodeIdValIdPrValNmH;

	v8::Persistent<v8::Function> OnValueCallback;

private:
	JsDeclareFunction(init);
	JsDeclareFunction(get);
	JsDeclareFunction(set);
	JsDeclareFunction(ping);

	// callbacks
	JsDeclareFunction(onValue);

	void OnMsgMainThread(const uint16& NodeId, const uint8& ValueId,
			const int& Val);

public:
	void OnValue(const uint16& NodeId, const int& ValId, const int& Val);

	class TOnMsgTask {
	private:
		TNodeJsRf24Radio* JsRadio;
		const uint16 NodeId;
		const int ValueId;
		const int Val;
	public:
		TOnMsgTask(TNodeJsRf24Radio* _JsRadio, const uint16& _NodeId, const int& _ValueId, const int& _Val):
			JsRadio(_JsRadio),
			NodeId(_NodeId),
			ValueId(_ValueId),
			Val(_Val) {}

		static void Run(TOnMsgTask& Task);
	};
};

#endif /* SRC_RPINODE_H_ */
