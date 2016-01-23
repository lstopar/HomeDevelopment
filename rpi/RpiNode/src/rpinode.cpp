#include "rpinode.h"

/////////////////////////////////////////
// DHT11 - Digital temperature and humidity sensor
void TNodejsDHT11Sensor::Init(v8::Handle<v8::Object> Exports) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(Isolate, TNodeJsUtil::_NewJs<TNodejsDHT11Sensor>);
	tpl->SetClassName(v8::String::NewFromUtf8(Isolate, GetClassId().CStr()));

	// ObjectWrap uses the first internal field to store the wrapped pointer.
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	// Add all methods, getters and setters here.
	NODE_SET_PROTOTYPE_METHOD(tpl, "init", _init);
	NODE_SET_PROTOTYPE_METHOD(tpl, "read", _read);
	NODE_SET_PROTOTYPE_METHOD(tpl, "readSync", _readSync);

	Exports->Set(v8::String::NewFromUtf8(Isolate, GetClassId().CStr()), tpl->GetFunction());
}

TNodejsDHT11Sensor* TNodejsDHT11Sensor::NewFromArgs(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	PJsonVal ParamJson = TNodeJsUtil::GetArgJson(Args, 0);

	const int Pin = ParamJson->GetObjInt("pin");
	const TStr& TempId = ParamJson->GetObjStr("temperatureId");
	const TStr& HumId = ParamJson->GetObjStr("humidityId");

	return new TNodejsDHT11Sensor(new TDHT11Sensor(Pin), TempId, HumId);
}

TNodejsDHT11Sensor::TNodejsDHT11Sensor(TDHT11Sensor* _Sensor, const TStr& _TempReadingId,
		const TStr& _HumReadingId):
			Sensor(_Sensor),
			TempReadingId(_TempReadingId),
			HumReadingId(_HumReadingId) {}

TNodejsDHT11Sensor::~TNodejsDHT11Sensor() {
	if (Sensor != nullptr) {
		delete Sensor;
	}
}

void TNodejsDHT11Sensor::init(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	TNodejsDHT11Sensor* JsSensor = ObjectWrap::Unwrap<TNodejsDHT11Sensor>(Args.Holder());
	JsSensor->Sensor->Init();

	Args.GetReturnValue().Set(v8::Undefined(Isolate));
}

TNodejsDHT11Sensor::TReadTask::TReadTask(const v8::FunctionCallbackInfo<v8::Value>& Args):
		TNodeTask(Args),
		JsSensor(nullptr) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	JsSensor = ObjectWrap::Unwrap<TNodejsDHT11Sensor>(Args.Holder());
}

v8::Handle<v8::Function> TNodejsDHT11Sensor::TReadTask::GetCallback(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	return TNodeJsUtil::GetArgFun(Args, 0);
}

v8::Local<v8::Value> TNodejsDHT11Sensor::TReadTask::WrapResult() {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::EscapableHandleScope HandleScope(Isolate);

	const double Temp = JsSensor->Sensor->GetTemp();
	const double Hum = JsSensor->Sensor->GetHum();

	v8::Local<v8::Object> RetVal = v8::Object::New(Isolate);
	RetVal->Set(v8::String::NewFromUtf8(Isolate, JsSensor->TempReadingId.CStr()), v8::Number::New(Isolate, Temp));
	RetVal->Set(v8::String::NewFromUtf8(Isolate, JsSensor->HumReadingId.CStr()), v8::Number::New(Isolate, Hum));

	return HandleScope.Escape(RetVal);
}

void TNodejsDHT11Sensor::TReadTask::Run() {
	try {
		JsSensor->Sensor->Read();
	} catch (const PExcept& Except) {
		SetExcept(Except);
	}
}

void TNodeJsYL40Adc::Init(v8::Handle<v8::Object> Exports) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(Isolate, TNodeJsUtil::_NewJs<TNodeJsYL40Adc>);
	tpl->SetClassName(v8::String::NewFromUtf8(Isolate, GetClassId().CStr()));

	// ObjectWrap uses the first internal field to store the wrapped pointer.
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	// Add all methods, getters and setters here.
	NODE_SET_PROTOTYPE_METHOD(tpl, "init", _init);
	NODE_SET_PROTOTYPE_METHOD(tpl, "setOutput", _setOutput);
	NODE_SET_PROTOTYPE_METHOD(tpl, "read", _read);
	NODE_SET_PROTOTYPE_METHOD(tpl, "readSync", _readSync);

	Exports->Set(v8::String::NewFromUtf8(Isolate, GetClassId().CStr()), tpl->GetFunction());
}

TNodeJsYL40Adc* TNodeJsYL40Adc::NewFromArgs(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	PJsonVal ArgJson = TNodeJsUtil::GetArgJson(Args, 0);
	PJsonVal InputJsonV = ArgJson->GetObjKey("inputs");

	TIntStrKdV InputNumNmKdV(InputJsonV->GetArrVals());
	for (int InputN = 0; InputN < InputJsonV->GetArrVals(); InputN++) {
		PJsonVal InputJson = InputJsonV->GetArrVal(InputN);
		InputNumNmKdV[InputN].Key = InputJson->GetObjInt("number");
		InputNumNmKdV[InputN].Dat = InputJson->GetObjStr("id");
	}

	return new TNodeJsYL40Adc(new TYL40Adc(), InputNumNmKdV);
}

TNodeJsYL40Adc::TNodeJsYL40Adc(TYL40Adc* _Adc, const TIntStrKdV& _InputNumNmKdV):
		Adc(_Adc),
		InputNumNmKdV(_InputNumNmKdV) {}

TNodeJsYL40Adc::~TNodeJsYL40Adc() {
	delete Adc;
}

void TNodeJsYL40Adc::init(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	TNodeJsYL40Adc* JsAdc = ObjectWrap::Unwrap<TNodeJsYL40Adc>(Args.Holder());
	JsAdc->Adc->Init();

	Args.GetReturnValue().Set(v8::Undefined(Isolate));
}

void TNodeJsYL40Adc::setOutput(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	TNodeJsYL40Adc* JsAdc = ObjectWrap::Unwrap<TNodeJsYL40Adc>(Args.Holder());
	const int OutputN = TNodeJsUtil::GetArgFlt(Args, 0);
	const double Perc = TNodeJsUtil::GetArgFlt(Args, 1);

	JsAdc->Adc->SetOutput(OutputN, 255 * Perc / 100);
	Args.GetReturnValue().Set(v8::Undefined(Isolate));
}

TNodeJsYL40Adc::TReadTask::TReadTask(const v8::FunctionCallbackInfo<v8::Value>& Args):
		TNodeTask(Args),
		Adc(nullptr) {
	Adc = ObjectWrap::Unwrap<TNodeJsYL40Adc>(Args.Holder());
}

v8::Handle<v8::Function> TNodeJsYL40Adc::TReadTask::GetCallback(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	return TNodeJsUtil::GetArgFun(Args, 0);
}

v8::Local<v8::Value> TNodeJsYL40Adc::TReadTask::WrapResult() {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::EscapableHandleScope HandleScope(Isolate);

	PJsonVal ResJson = TJsonVal::NewObj();
	for (int InputN = 0; InputN < Adc->InputNumNmKdV.Len(); InputN++) {
		ResJson->AddToObj(Adc->InputNumNmKdV[InputN].Dat, ValV[InputN]);
	}

	return HandleScope.Escape(TNodeJsUtil::ParseJson(Isolate, ResJson));
}

void TNodeJsYL40Adc::TReadTask::Run() {
	try {
		const TIntStrKdV& InputNumNmKdV = Adc->InputNumNmKdV;
		ValV.Gen(InputNumNmKdV.Len());

		for (int InputN = 0; InputN < InputNumNmKdV.Len(); InputN++) {
			int Val = Adc->Adc->Read(InputNumNmKdV[InputN].Key);
			ValV[InputN] = Val;
		}
	} catch (const PExcept& Except) {
		SetExcept(Except);
	}
}


//////////////////////////////////////////////
// module initialization
void Init(v8::Handle<v8::Object> Exports) {
	TNodejsDHT11Sensor::Init(Exports);
	TNodeJsYL40Adc::Init(Exports);
}

NODE_MODULE(rpinode, Init);
