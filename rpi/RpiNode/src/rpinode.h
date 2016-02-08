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
	JsDeclareFunction(pinMode);
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
	static TRf24Radio* NewFromArgs(const v8::FunctionCallbackInfo<v8::Value>& Args);

	~TNodeJsRf24Radio();

	TRf24Radio* Radio;
	THash<TStr, uint8> ValueNmIdH;

	v8::Persistent<v8::Function> MsgCallback;

private:
	JsDeclareFunction(init);
	JsDeclareFunction(get);
	JsDeclareFunction(set);
	JsDeclareFunction(onMsg);

	void OnMsgMainThread(const int& NodeId, const uint8& ValueId, const TMem& Msg);

protected:
	void OnMsg(const TMem& Msg);

	class TOnMsgTask {
	private:
		TNodeJsRf24Radio* JsRadio;
		const int NodeId;
		const uint8 ValueId;
		const TMem Msg;
	public:
		TOnMsgTask(TNodeJsRf24Radio* _JsRadio, const int& _NodeId, const uint8& _ValueId,
				const TMem& _Msg):
			JsRadio(_JsRadio),
			NodeId(_NodeId),
			ValueId(_ValueId),
			Msg(_Msg) {}

		void Run(TOnMsgTask& Task);
	};
};

#endif /* SRC_RPINODE_H_ */
