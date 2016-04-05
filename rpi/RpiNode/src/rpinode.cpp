#include "rpinode.h"

void TNodeJsRpiBase::Init(v8::Handle<v8::Object> Exports) {
	NODE_SET_METHOD(Exports, "init", _init);
//	NODE_SET_METHOD(Exports, "pinMode", _pinMode);
}

void TNodeJsRpiBase::init(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	PJsonVal ArgJson = Args.Length() > 0 ? TNodeJsUtil::GetArgJson(Args, 1) : TJsonVal::NewObj();

	const TStr PinLayoutStr = ArgJson->GetObjStr("pinLayout", "bcmGpio");

	TGpioLayout PinLayout;
	if (PinLayoutStr == "bcmGpio") {
		PinLayout = TGpioLayout::glBcmGpio;
	} else if (PinLayoutStr == "wiringPi") {
		PinLayout = TGpioLayout::glWiringPi;
	} else {
		throw TExcept::New("Invalid pin layout: " + PinLayoutStr);
	}

	TRpiUtil::InitGpio(PinLayout);
	Args.GetReturnValue().Set(v8::Undefined(Isolate));
}

//void TNodeJsRpiBase::pinMode(const v8::FunctionCallbackInfo<v8::Value>& Args) {
//	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
//	v8::HandleScope HandleScope(Isolate);
//
//	const int Pin = TNodeJsUtil::GetArgInt32(Args, 0);
//	const int Mode = TNodeJsUtil::GetArgInt32(Args, 1);
//
//	TRpiUtil::SetPinMode(Pin, Mode);
//	Args.GetReturnValue().Set(v8::Undefined(Isolate));
//}

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
	const uint64 MxSampleTm = ParamJson->GetObjUInt64("timeout", 10000);
	const bool Verbose = ParamJson->GetObjBool("verbose", false);

	const PNotify Notify = Verbose ? TNotify::StdNotify : TNotify::NullNotify;

	return new TNodejsDHT11Sensor(new TDHT11Sensor(Pin, Notify), TempId, HumId, MxSampleTm, Notify);
}

TNodejsDHT11Sensor::TNodejsDHT11Sensor(TDHT11Sensor* _Sensor, const TStr& _TempReadingId,
		const TStr& _HumReadingId, const uint64& _MxSampleTm, const PNotify& _Notify):
			Sensor(_Sensor),
			TempReadingId(_TempReadingId),
			HumReadingId(_HumReadingId),
			MxSampleTm(_MxSampleTm),
			Notify(_Notify) {}

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
	const uint64 StartTm = TTm::GetCurUniMSecs();

	int RetryN = 0;
	bool Success = false;
	while (true) {
		RetryN++;

		try {
			JsSensor->Sensor->Read();
			Success = true;
			break;
		} catch (const PExcept& Except) {
			const uint64 TimeExpired = TTm::GetCurUniMSecs() - StartTm;
			const int64 TimeLeft = int64(JsSensor->MxSampleTm) - int64(TimeExpired);

			JsSensor->Notify->OnNotifyFmt(TNotifyType::ntInfo, "Failed to read DHT11: %s, time left %ld ...", Except->GetMsgStr().CStr(), (RetryN+1));

			if (TimeLeft - int64(TDHT11Sensor::SAMPLING_TM) <= 0) { break; }

			// sleep for 2 seconds before reading again
			JsSensor->Notify->OnNotifyFmt(TNotifyType::ntInfo, "Attempt %d in 2 seconds ...", (RetryN+1));
			TSysProc::Sleep(TDHT11Sensor::MIN_SAMPLING_PERIOD);
		}
	}


	if (!Success) {
		SetExcept(TExcept::New("Failed to read DHT11 after " + TUInt64::GetStr(JsSensor->MxSampleTm)));
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

	PJsonVal ParamJson = TNodeJsUtil::GetArgJson(Args, 0);
	PJsonVal InputJsonV = ParamJson->GetObjKey("inputs");
	const bool Verbose = ParamJson->GetObjBool("verbose", false);

	TIntStrKdV InputNumNmKdV(InputJsonV->GetArrVals());
	for (int InputN = 0; InputN < InputJsonV->GetArrVals(); InputN++) {
		PJsonVal InputJson = InputJsonV->GetArrVal(InputN);
		InputNumNmKdV[InputN].Key = InputJson->GetObjInt("number");
		InputNumNmKdV[InputN].Dat = InputJson->GetObjStr("id");
	}

	const PNotify Notify = Verbose ? TNotify::StdNotify : TNotify::NullNotify;

	return new TNodeJsYL40Adc(new TYL40Adc(Notify), InputNumNmKdV, Notify);
}

TNodeJsYL40Adc::TNodeJsYL40Adc(TYL40Adc* _Adc, const TIntStrKdV& _InputNumNmKdV, const PNotify& _Notify):
		Adc(_Adc),
		InputNumNmKdV(_InputNumNmKdV),
		Notify(_Notify) {}

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

		Adc->Notify->OnNotifyFmt(TNotifyType::ntInfo, "YL-40 will read %d inputs ...", InputNumNmKdV.Len());
		ValV.Gen(InputNumNmKdV.Len());
		for (int InputN = 0; InputN < InputNumNmKdV.Len(); InputN++) {
			int Val = Adc->Adc->Read(InputNumNmKdV[InputN].Key);
			ValV[InputN] = Val;
		}
	} catch (const PExcept& Except) {
		SetExcept(Except);
	}
}

/////////////////////////////////////////
// RF24 - Radio
void TNodeJsRf24Radio::Init(v8::Handle<v8::Object> Exports) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(Isolate, TNodeJsUtil::_NewJs<TNodeJsRf24Radio>);
	tpl->SetClassName(v8::String::NewFromUtf8(Isolate, GetClassId().CStr()));

	// ObjectWrap uses the first internal field to store the wrapped pointer.
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	// Add all methods, getters and setters here.
	NODE_SET_PROTOTYPE_METHOD(tpl, "init", _init);
	NODE_SET_PROTOTYPE_METHOD(tpl, "get", _get);
	NODE_SET_PROTOTYPE_METHOD(tpl, "getAll", _getAll);
	NODE_SET_PROTOTYPE_METHOD(tpl, "set", _set);
	NODE_SET_PROTOTYPE_METHOD(tpl, "ping", _ping);
	NODE_SET_PROTOTYPE_METHOD(tpl, "onValue", _onValue);

	Exports->Set(v8::String::NewFromUtf8(Isolate, GetClassId().CStr()), tpl->GetFunction());
}

TNodeJsRf24Radio* TNodeJsRf24Radio::NewFromArgs(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	PJsonVal ParamJson = TNodeJsUtil::GetArgJson(Args, 0);

	const int PinCE = ParamJson->GetObjInt("pinCE");
	const int PinCSN = ParamJson->GetObjInt("pinCSN");
	const uint16 MyId = (uint16) ParamJson->GetObjInt("id");
	const PJsonVal SensorJsonV = ParamJson->GetObjKey("sensors");

	const bool Verbose = ParamJson->GetObjBool("verbose", false);
	const PNotify Notify = Verbose ? TNotify::StdNotify : TNotify::NullNotify;

	Notify->OnNotify(TNotifyType::ntInfo, "Parsing configuration ...");

	TStrIntH SensorNmIdH;
	TStrIntH SensorIdNodeIdH;

	for (int SensorN = 0; SensorN < SensorJsonV->GetArrVals(); SensorN++) {
		const PJsonVal SensorJson = SensorJsonV->GetArrVal(SensorN);
		const TStr& SensorId = SensorJson->GetObjStr("id");
		SensorNmIdH.AddDat(SensorId, SensorJson->GetObjInt("internalId"));
		SensorIdNodeIdH.AddDat(SensorId, SensorJson->GetObjInt("nodeId"));
	}

	Notify->OnNotify(TNotifyType::ntInfo, "Calling cpp constructor ...");

	return new TNodeJsRf24Radio(MyId, PinCE, PinCSN, SensorNmIdH, SensorIdNodeIdH, Notify);
}

TNodeJsRf24Radio::TNodeJsRf24Radio(const uint16& NodeId, const int& PinCE, const int& PinCSN,
		const TStrIntH& ValueNmIdH, const TStrIntH& ValueNmNodeIdH,
		const PNotify& _Notify):
	Radio(NodeId, PinCE, PinCSN, BCM2835_SPI_SPEED_8MHZ, _Notify),
	ValNmNodeIdValIdPrH(),
	NodeIdValIdPrValNmH(),
	OnValueCallback(),
	Notify(_Notify) {

	Notify->OnNotify(TNotifyType::ntInfo, "Setting radio cpp callback ...");
	Radio.SetCallback(this);

	Notify->OnNotify(TNotifyType::ntInfo, "Initializing Id conversion structures ...");
	int KeyId = ValueNmIdH.FFirstKeyId();
	while (ValueNmIdH.FNextKeyId(KeyId)) {
		const TStr& ValNm = ValueNmIdH.GetKey(KeyId);

		const int& ValId = ValueNmIdH[KeyId];
		const int& NodeId = ValueNmNodeIdH.GetDat(ValNm);

		ValNmNodeIdValIdPrH.AddDat(ValNm, TIntPr(NodeId, ValId));
		NodeIdValIdPrValNmH.AddDat(TIntPr(NodeId, ValId), ValNm);
	}
}

TNodeJsRf24Radio::~TNodeJsRf24Radio() {
	OnValueCallback.Reset();
}

void TNodeJsRf24Radio::init(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	TNodeJsRf24Radio* JsRadio = ObjectWrap::Unwrap<TNodeJsRf24Radio>(Args.Holder());
	JsRadio->Radio.Init();

	Args.GetReturnValue().Set(v8::Undefined(Isolate));
}

void TNodeJsRf24Radio::get(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	TNodeJsRf24Radio* JsRadio = ObjectWrap::Unwrap<TNodeJsRf24Radio>(Args.Holder());

	const TStr ValueNm = TNodeJsUtil::GetArgStr(Args, 0);
	const TIntPr& NodeIdValIdPr = JsRadio->ValNmNodeIdValIdPrH.GetDat(ValueNm);

	const bool Success = JsRadio->Radio.Get((uint16) NodeIdValIdPr.Val1, NodeIdValIdPr.Val2);

	Args.GetReturnValue().Set(v8::Boolean::New(Isolate, Success));
}

void TNodeJsRf24Radio::getAll(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	TNodeJsRf24Radio* JsRadio = ObjectWrap::Unwrap<TNodeJsRf24Radio>(Args.Holder());
	const int NodeId = TNodeJsUtil::GetArgInt32(Args, 0);

	const bool Success = JsRadio->Radio.GetAll((uint16) NodeId);

	Args.GetReturnValue().Set(v8::Boolean::New(Isolate, Success));
}

void TNodeJsRf24Radio::set(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	TNodeJsRf24Radio* JsRadio = ObjectWrap::Unwrap<TNodeJsRf24Radio>(Args.Holder());

	if (Args.Length() == 0) { return; }

	const PJsonVal ArgVal = TNodeJsUtil::GetArgJson(Args, 0);
	bool Success = true;

	if (ArgVal->IsArr()) {
		THash<TInt, TIntPrV> NodeIdValIdValPrVH;

		for (int ArgN = 0; ArgN < ArgVal->GetArrVals(); ArgN++) {
			const PJsonVal& ValJson = ArgVal->GetArrVal(ArgN);

			const TStr& ValNm = ValJson->GetObjStr("sensorId");
			const int& Val = ValJson->GetObjInt("value");

			const TIntPr& NodeIdValIdPr = JsRadio->ValNmNodeIdValIdPrH.GetDat(ValNm);
			const uint16 NodeId = NodeIdValIdPr.Val1;
			const int ValId = NodeIdValIdPr.Val2;

			if (!NodeIdValIdValPrVH.IsKey(NodeId)) { NodeIdValIdValPrVH.AddDat(NodeId); }

			TIntPrV& ValIdValPrV = NodeIdValIdValPrVH.GetDat(NodeId);
			ValIdValPrV.Add(TIntPr(ValId, Val));
		}

		int KeyId = NodeIdValIdValPrVH.FFirstKeyId();
		while (NodeIdValIdValPrVH.FNextKeyId(KeyId)) {
			const uint16 NodeId = NodeIdValIdValPrVH.GetKey(KeyId);
			const TIntPrV& ValIdValPrV = NodeIdValIdValPrVH[KeyId];
			Success &= JsRadio->Radio.Set(NodeId, ValIdValPrV);
		}
	} else {
		const TStr& ValueNm = ArgVal->GetObjStr("sensorId");
		const int Val = ArgVal->GetObjInt("value");

		const TIntPr& NodeIdValIdPr = JsRadio->ValNmNodeIdValIdPrH.GetDat(ValueNm);
		const uint16 NodeId = (uint16) NodeIdValIdPr.Val1;
		const int ValId = NodeIdValIdPr.Val2;

		Success = JsRadio->Radio.Set(NodeId, ValId, Val);
	}

	Args.GetReturnValue().Set(v8::Boolean::New(Isolate, Success));
}

void TNodeJsRf24Radio::ping(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	TNodeJsRf24Radio* JsRadio = ObjectWrap::Unwrap<TNodeJsRf24Radio>(Args.Holder());

	const int NodeId = (uint16) TNodeJsUtil::GetArgInt32(Args, 0);

	bool Success = JsRadio->Radio.Ping(NodeId);

	Args.GetReturnValue().Set(v8::Boolean::New(Isolate, Success));
}

void TNodeJsRf24Radio::onValue(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	TNodeJsRf24Radio* JsRadio = ObjectWrap::Unwrap<TNodeJsRf24Radio>(Args.Holder());
	v8::Local<v8::Function> Cb = TNodeJsUtil::GetArgFun(Args, 0);

	JsRadio->OnValueCallback.Reset(Isolate, Cb);
	Args.GetReturnValue().Set(v8::Undefined(Isolate));
}

void TNodeJsRf24Radio::OnMsgMainThread(const uint16& NodeId, const uint8& ValueId,
		const int& Val) {
	if (!OnValueCallback.IsEmpty()) {
		v8::Isolate* Isolate = v8::Isolate::GetCurrent();
		v8::HandleScope HandleScope(Isolate);

		const int ValId = (int) ValueId;
		Notify->OnNotifyFmt(TNotifyType::ntInfo, "Got value for value id %d", ValId);

		TIntPr NodeIdValIdPr(NodeId, (int) ValId);
		EAssertR(NodeIdValIdPrValNmH.IsKey(NodeIdValIdPr), "Node-valueId pair not stored in the structures!");

		const TStr& ValueNm = NodeIdValIdPrValNmH.GetDat(NodeIdValIdPr);

		PJsonVal JsonVal = TJsonVal::NewObj();
		JsonVal->AddToObj("id", ValueNm);
		JsonVal->AddToObj("value", Val);

		v8::Local<v8::Function> Callback = v8::Local<v8::Function>::New(Isolate, OnValueCallback);
		TNodeJsUtil::ExecuteVoid(Callback, TNodeJsUtil::ParseJson(Isolate, JsonVal));
	}
}

void TNodeJsRf24Radio::OnValue(const uint16& NodeId, const char& ValId, const int& Val) {
	Notify->OnNotifyFmt(TNotifyType::ntInfo, "Executing callback for value id: %d", ValId);
	TNodeJsAsyncUtil::ExecuteOnMainAndWait(new TOnMsgTask(this, NodeId, (int) ValId, Val), true);
}

void TNodeJsRf24Radio::TOnMsgTask::Run() {
	try {
		JsRadio->OnMsgMainThread(NodeId, ValueId, Val);
	} catch (const PExcept& Except) {
		JsRadio->Notify->OnNotifyFmt(TNotifyType::ntErr, "Failed to execute value callback: %s!", Except->GetMsgStr().CStr());
	}
}


//////////////////////////////////////////////
// module initialization
void Init(v8::Handle<v8::Object> Exports) {
	TNodeJsRpiBase::Init(Exports);
	TNodejsDHT11Sensor::Init(Exports);
	TNodeJsYL40Adc::Init(Exports);
	TNodeJsRf24Radio::Init(Exports);
}

NODE_MODULE(rpinode, Init);
