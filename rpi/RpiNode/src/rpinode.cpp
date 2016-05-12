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
	NODE_SET_PROTOTYPE_METHOD(tpl, "on", _on);

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
	NodeIdSet(),
	PongQ(),
	ValQ(),
	OnValueCallback(),
	OnPongCallback(),
	CallbackHandle(TNodeJsAsyncUtil::NewHandle()),
	CallbackSection(),
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
		NodeIdSet.AddKey(NodeId);
	}
}

TNodeJsRf24Radio::~TNodeJsRf24Radio() {
	OnValueCallback.Reset();
	OnPongCallback.Reset();

	TNodeJsAsyncUtil::DelHandle(CallbackHandle);
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

		JsRadio->Notify->OnNotifyFmt(ntInfo, "Setting %d values ...", ArgVal->GetArrVals());

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

void TNodeJsRf24Radio::on(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	TNodeJsRf24Radio* JsRadio = ObjectWrap::Unwrap<TNodeJsRf24Radio>(Args.Holder());
	const TStr EventNm = TNodeJsUtil::GetArgStr(Args, 0);

	if (EventNm == "value") {
		if (TNodeJsUtil::IsArgNullOrUndef(Args, 1)) {
			JsRadio->OnValueCallback.Reset();
		}
		else {
			v8::Local<v8::Function> Cb = TNodeJsUtil::GetArgFun(Args, 1);
			JsRadio->OnValueCallback.Reset(Isolate, Cb);
		}
	}
	else if (EventNm == "pong") {
		if (TNodeJsUtil::IsArgNullOrUndef(Args, 1)) {
			JsRadio->OnPongCallback.Reset();
		}
		else {
			v8::Local<v8::Function> Cb = TNodeJsUtil::GetArgFun(Args, 1);
			JsRadio->OnPongCallback.Reset(Isolate, Cb);
		}
	}
	else {
		throw TExcept::New("Unknown event: " + EventNm);
	}

	Args.GetReturnValue().Set(v8::Undefined(Isolate));
}

void TNodeJsRf24Radio::ProcessQueues() {
	TVec<TUInt16> TempPongQ;
	TVec<TTriple<TUInt16, TCh, TInt>> TempValQ;

	{
		TLock Lock(CallbackSection);

		TempPongQ = PongQ;
		TempValQ = ValQ;

		PongQ.Clr();
		ValQ.Clr();
	}

	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	if (!TempPongQ.Empty() && !OnPongCallback.IsEmpty()) {
		v8::Local<v8::Function> Callback = v8::Local<v8::Function>::New(Isolate, OnPongCallback);

		Notify->OnNotifyFmt(ntInfo, "Processing %d pong requests ...", TempPongQ.Len());
		for (int PongN = 0; PongN < TempPongQ.Len(); PongN++) {
			const uint16& NodeId = TempPongQ[PongN];

			Notify->OnNotifyFmt(TNotifyType::ntInfo, "Got pong from node %u", NodeId);
			EAssertR(NodeIdSet.IsKey(NodeId), "Received invalid node ID: " + TUInt::GetStr(NodeId));

			TNodeJsUtil::ExecuteVoid(Callback, v8::Integer::New(Isolate, (int) NodeId));
		}
	}

	if (!TempValQ.Empty() && !OnValueCallback.IsEmpty()) {
		v8::Local<v8::Function> Callback = v8::Local<v8::Function>::New(Isolate, OnValueCallback);

		Notify->OnNotifyFmt(ntInfo, "Processing %d value requests ...", TempValQ.Len());
		for (int ValN = 0; ValN < TempValQ.Len(); ValN++) {
			const TTriple<TUInt16, TCh, TInt>& ValTr = TempValQ[ValN];

			const uint16& NodeId = ValTr.Val1;
			const char ValId = ValTr.Val2;
			const int& Val = ValTr.Val3;

			TIntPr NodeIdValIdPr(NodeId, (int) ValId);
			EAssertR(NodeIdValIdPrValNmH.IsKey(NodeIdValIdPr), "Node-valueId pair not stored in the structures!");

			const TStr& ValueNm = NodeIdValIdPrValNmH.GetDat(NodeIdValIdPr);

			PJsonVal JsonVal = TJsonVal::NewObj();
			JsonVal->AddToObj("id", ValueNm);
			JsonVal->AddToObj("value", Val);

			TNodeJsUtil::ExecuteVoid(Callback, TNodeJsUtil::ParseJson(Isolate, JsonVal));
		}
	}
}

void TNodeJsRf24Radio::OnPong(const uint16& NodeId) {
	{
		TLock Lock(CallbackSection);
		PongQ.Add(NodeId);
	}

	TNodeJsAsyncUtil::ExecuteOnMain(new TProcessQueuesTask(this), CallbackHandle, true);
}

void TNodeJsRf24Radio::OnValV(const TVec<TTriple<TUInt16, TCh, TInt>>& NodeIdValIdValV) {
	Notify->OnNotifyFmt(TNotifyType::ntInfo, "Received value %d values in wrapper ...", NodeIdValIdValV.Len());

	{
		TLock Lock(CallbackSection);
		ValQ.AddV(NodeIdValIdValV);
	}

	TNodeJsAsyncUtil::ExecuteOnMain(new TProcessQueuesTask(this), CallbackHandle, true);
}

void TNodeJsRf24Radio::TProcessQueuesTask::Run() {
	try {
		JsRadio->ProcessQueues();
	} catch (const PExcept& Except) {
		JsRadio->Notify->OnNotifyFmt(TNotifyType::ntErr, "Failed to execute value callback: %s!", Except->GetMsgStr().CStr());
	}
}

TNodeJsEoDevice::TNodeJsEoDevice(const uint32& _DeviceId, TEoGateway* _Gateway,
		const PNotify& _Notify):
		DeviceId(_DeviceId),
		Gateway(_Gateway),
		Notify(_Notify) {
	Notify->OnNotifyFmt(ntInfo, "Device %u created!", DeviceId);
}

eoDevice* TNodeJsEoDevice::GetDevice() const {
	eoDevice* Device = Gateway->GetDevice(DeviceId);
	EAssertR(Device != nullptr, "Device missing in gateway!");
	return Device;
}

/////////////////////////////////////////////
// EnOcean F6-02 double button wall rocker
v8::Persistent<v8::Function> TNodeJsF602Rocker::Constructor;

void TNodeJsF602Rocker::Init(v8::Handle<v8::Object> exports) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);
	// template for creating function from javascript using "new", uses _NewJs callback
	v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(Isolate, TNodeJsUtil::_NewCpp<TNodeJsF602Rocker>);


	tpl->SetClassName(v8::String::NewFromUtf8(Isolate, GetClassId().CStr()));
	// ObjectWrap uses the first internal field to store the wrapped pointer
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	NODE_SET_PROTOTYPE_METHOD(tpl, "on", _on);

	tpl->InstanceTemplate()->SetAccessor(v8::String::NewFromUtf8(Isolate, "id"), _id);
	tpl->InstanceTemplate()->SetAccessor(v8::String::NewFromUtf8(Isolate, "type"), _type);

	Constructor.Reset(Isolate, tpl->GetFunction());

	// we need to export the class for calling using "new FIn(...)"
	exports->Set(v8::String::NewFromUtf8(Isolate, GetClassId().CStr()),
		tpl->GetFunction());
}

TNodeJsF602Rocker::~TNodeJsF602Rocker() {
	MsgCallback.Reset();
}

void TNodeJsF602Rocker::OnMsg(const eoMessage& Msg) {
	eoEEP_F602xx Profile;
	Profile.SetType(0x01);

	EAssertR(Profile.Parse(Msg) == EO_OK, "Failed to parse F6-02 rocker message!");

	uchar RockerA, RockerB;

	EAssertR(Profile.GetValue(E_ROCKER_A, RockerA) == EO_OK, "Failed to get the value of rocker A!");
	EAssertR(Profile.GetValue(E_ROCKER_B, RockerB) == EO_OK, "Failed to get the value of rocker B!");

	// TODO E_ENERGYBOW
	// TODO E_MULTIPRESS

	if (RockerA != STATE_NP) {
		OnRocker(0, RockerA == STATE_I);
	}
	if (RockerB != STATE_NP) {
		OnRocker(1, RockerB == STATE_I);
	}
}

void TNodeJsF602Rocker::id(v8::Local<v8::String> Name, const v8::PropertyCallbackInfo<v8::Value>& Info) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	TNodeJsF602Rocker* JsDevice = ObjectWrap::Unwrap<TNodeJsF602Rocker>(Info.Holder());
	Info.GetReturnValue().Set(v8::Number::New(Isolate, JsDevice->GetDeviceId()));
}

void TNodeJsF602Rocker::type(v8::Local<v8::String> Name, const v8::PropertyCallbackInfo<v8::Value>& Info) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	Info.GetReturnValue().Set(v8::String::NewFromUtf8(Isolate, "F6-02-xx"));
}

void TNodeJsF602Rocker::on(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);
	// unwrap
	TNodeJsF602Rocker* JsDevice = ObjectWrap::Unwrap<TNodeJsF602Rocker>(Args.Holder());

	const TStr EventId = TNodeJsUtil::GetArgStr(Args, 0);

	if (EventId == "button") {
		v8::Local<v8::Function> Cb = TNodeJsUtil::GetArgFun(Args, 1);
		JsDevice->MsgCallback.Reset(Isolate, Cb);
	} else {
		throw TExcept::New("EventId: " + EventId + " not supported!");
	}

	Args.GetReturnValue().Set(v8::Undefined(Isolate));
}

void TNodeJsF602Rocker::OnRocker(const int& RockerId, const bool& Pressed) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	if (!MsgCallback.IsEmpty()) {
		v8::Local<v8::Function> Callback = v8::Local<v8::Function>::New(Isolate, MsgCallback);
		TNodeJsUtil::ExecuteVoid(Callback, v8::Integer::New(Isolate, RockerId), v8::Boolean::New(Isolate, Pressed));
	}
}

/////////////////////////////////////////////
// EnOcean D2-01 device
v8::Persistent<v8::Function> TNodeJsD201Device::Constructor;

void TNodeJsD201Device::Init(v8::Handle<v8::Object> exports) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);
	// template for creating function from javascript using "new", uses _NewJs callback
	v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(Isolate, TNodeJsUtil::_NewCpp<TNodeJsD201Device>);


	tpl->SetClassName(v8::String::NewFromUtf8(Isolate, GetClassId().CStr()));
	// ObjectWrap uses the first internal field to store the wrapped pointer
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	NODE_SET_PROTOTYPE_METHOD(tpl, "setOutput", _setOutput);
	NODE_SET_PROTOTYPE_METHOD(tpl, "readStatus", _readStatus);
	NODE_SET_PROTOTYPE_METHOD(tpl, "on", _on);

	tpl->InstanceTemplate()->SetAccessor(v8::String::NewFromUtf8(Isolate, "id"), _id);
	tpl->InstanceTemplate()->SetAccessor(v8::String::NewFromUtf8(Isolate, "type"), _type);

	Constructor.Reset(Isolate, tpl->GetFunction());

	// we need to export the class for calling using "new FIn(...)"
	exports->Set(v8::String::NewFromUtf8(Isolate, GetClassId().CStr()),
		tpl->GetFunction());
}

TNodeJsD201Device::~TNodeJsD201Device() {
	StatusCb.Reset();
}

void TNodeJsD201Device::setOutput(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);
	// unwrap
	TNodeJsD201Device* JsDevice = ObjectWrap::Unwrap<TNodeJsD201Device>(Args.Holder());
	TEoGateway* Gateway = JsDevice->GetGateway();

	const uint8 ChannelN = (uint8) TNodeJsUtil::GetArgInt32(Args, 0);
	const uint8 Value = (uint8) TNodeJsUtil::GetArgInt32(Args, 1);

	JsDevice->Notify->OnNotifyFmt(ntInfo, "Setting output deviceId: %u, channel: 0x%02x, value: %d ...", JsDevice->DeviceId, ChannelN, Value);

	eoMessage Msg(eoEEP_D201xx::MX_LEN);
	eoReturn Code = eoEEP_D201xx::CreateSetOutput(
			Gateway->GetId(),
			JsDevice->DeviceId,
			0x00,
			ChannelN,
			Value,
			Msg
	);

	EAssertR(Code == EO_OK, "Failed to generate setOutput message: " + TUInt::GetStr(Code) + "!");

	Gateway->Send(Msg);

	Args.GetReturnValue().Set(v8::Undefined(Isolate));
}


void TNodeJsD201Device::readStatus(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);
	// unwrap
	TNodeJsD201Device* JsDevice = ObjectWrap::Unwrap<TNodeJsD201Device>(Args.Holder());
	TEoGateway* Gateway = JsDevice->GetGateway();

	const int ChannelN = TNodeJsUtil::GetArgInt32(Args, 0, -1);
	const uint8 Channel = ChannelN >= 0 ? (uint8) ChannelN : 0x1E;

	const uint32& GatewayId = Gateway->GetId();
	const uint32& DeviceId = JsDevice->DeviceId;

	JsDevice->Notify->OnNotifyFmt(ntInfo, "Reading status of device %u, sourceId: %u, channel: 0x%02x ...", DeviceId, GatewayId, Channel);

	eoMessage Msg(eoEEP_D201xx::MX_LEN);
	eoReturn Code = eoEEP_D201xx::CreateStatusQuery(GatewayId, DeviceId, Channel, Msg);

	EAssertR(Code == EO_OK, "Failed to generate readStatus message: " + TUInt::GetStr(Code) + "!");

	Gateway->Send(Msg);

	Args.GetReturnValue().Set(v8::Undefined(Isolate));
}

void TNodeJsD201Device::on(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);
	// unwrap
	TNodeJsD201Device* JsDevice = ObjectWrap::Unwrap<TNodeJsD201Device>(Args.Holder());

	const TStr EventId = TNodeJsUtil::GetArgStr(Args, 0);

	if (EventId == "status") {
		v8::Local<v8::Function> Cb = TNodeJsUtil::GetArgFun(Args, 1);
		JsDevice->StatusCb.Reset(Isolate, Cb);
	} else {
		throw TExcept::New("EventId: " + EventId + " not supported!");
	}

	Args.GetReturnValue().Set(v8::Undefined(Isolate));
}

void TNodeJsD201Device::id(v8::Local<v8::String> Name, const v8::PropertyCallbackInfo<v8::Value>& Info) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	TNodeJsD201Device* JsDevice = ObjectWrap::Unwrap<TNodeJsD201Device>(Info.Holder());
	Info.GetReturnValue().Set(v8::Number::New(Isolate, JsDevice->DeviceId));
}

void TNodeJsD201Device::type(v8::Local<v8::String> Name, const v8::PropertyCallbackInfo<v8::Value>& Info) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	Info.GetReturnValue().Set(v8::String::NewFromUtf8(Isolate, "D2-01-xx"));
}

void TNodeJsD201Device::OnMsg(const eoMessage& Msg) {
	try {
		Notify->OnNotify(ntInfo, "Got enocean message in JsDevice ...");
		d2_01Command Command = eoEEP_D201xx::GetCommand(Msg);

		switch (Command) {
		case ACTUATOR_STATUS_RESPONSE: {
			eoEEP_D201xx Profile;
			Profile.Parse(Msg);

			uint8 Channel, Value;
			Profile.GetValue(E_IO_CHANNEL, Channel);
			Profile.GetValue(F_ON_OFF, Value);
			OnStatus(Channel, Value);
			break;
		}
		case ACTUATOR_MEASUREMENT_RESPONSE: {
			// TODO
			break;
		}
		case ACTUATOR_PILOT_WIRE_MODE_RESPONSE: {
			// TODO
			break;
		}
		case ACTUATOR_SET_EXTERNAL_INTERFACE_SETTINGS_RESPONSE: {
			// TODO
			break;
		}
		default: {
			throw TExcept::New("Unhandled command: " + TCh::GetHex(Command));
		}
		}
	} catch (...) {
		Notify->OnNotifyFmt(ntErr, "Unknown exception in EnOcean device!");
	}
}

void TNodeJsD201Device::OnStatus(const uint8& Channel, const uint8& Val) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	Notify->OnNotifyFmt(ntInfo, "Got message in OnStatus, channel: %u, value: %u ...", Channel, Val);

	if (!StatusCb.IsEmpty()) {
		Notify->OnNotify(ntInfo, "Executing EnOcean callback ...");
		v8::Local<v8::Function> Callback = v8::Local<v8::Function>::New(Isolate, StatusCb);
		TNodeJsUtil::ExecuteVoid(Callback, v8::Integer::New(Isolate, (int) Channel), v8::Integer::New(Isolate, (int) Val));
		Notify->OnNotify(ntInfo, "EnOcean callback executed!");
	}
	else {
		Notify->OnNotify(ntErr, "EnOcean callback is empty!");
	}
}


/////////////////////////////////////////////
// EnOcean Gateway
void TNodeJsEoGateway::Init(v8::Handle<v8::Object> Exports) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(Isolate, TNodeJsUtil::_NewJs<TNodeJsEoGateway>);
	tpl->SetClassName(v8::String::NewFromUtf8(Isolate, GetClassId().CStr()));

	// ObjectWrap uses the first internal field to store the wrapped pointer.
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	// Add all methods, getters and setters here.
	NODE_SET_PROTOTYPE_METHOD(tpl, "init", _init);
	NODE_SET_PROTOTYPE_METHOD(tpl, "startLearningMode", _startLearningMode);
	NODE_SET_PROTOTYPE_METHOD(tpl, "on", _on);

	Exports->Set(v8::String::NewFromUtf8(Isolate, GetClassId().CStr()), tpl->GetFunction());
}

TNodeJsEoGateway* TNodeJsEoGateway::NewFromArgs(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	const PJsonVal ParamJson = TNodeJsUtil::GetArgJson(Args, 0);

	const TStr SerialPort = ParamJson->GetObjStr("serialPort");
	const TStr StorageFNm = ParamJson->GetObjStr("storageFile");
	const PNotify Notify = ParamJson->GetObjBool("verbose", false) ? TNotify::StdNotify : TNotify::NullNotify;

	Notify->OnNotifyFmt(TNotifyType::ntInfo, "Parsed arguments, creating EnOcean gateway: %s, %s ...", StorageFNm.CStr(), SerialPort.CStr());

	return new TNodeJsEoGateway(SerialPort, StorageFNm, Notify);
}

TNodeJsEoGateway::TNodeJsEoGateway(const TStr& SerialPort, const TStr& StorageFNm,
		const PNotify& _Notify):
		Gateway(SerialPort, StorageFNm, _Notify),
		DeviceMap(),
		OnDeviceCallback(),
		NewDeviceIdQ(),
		DeviceIdMsgPrQ(),
		CallbackHandle(TNodeJsAsyncUtil::NewHandle()),
		CallbackSection(),
		Notify(_Notify) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	Notify->OnNotify(TNotifyType::ntInfo, "Initializing device map ...");
	DeviceMap.Reset(Isolate, v8::Object::New(Isolate));

	Gateway.SetCallback(this);
}

TNodeJsEoGateway::~TNodeJsEoGateway() {
	DeviceMap.Reset();
	OnDeviceCallback.Reset();

	TNodeJsAsyncUtil::DelHandle(CallbackHandle);
}

void TNodeJsEoGateway::init(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);
	// unwrap
	TNodeJsEoGateway* JsGateway = ObjectWrap::Unwrap<TNodeJsEoGateway>(Args.Holder());

	JsGateway->Notify->OnNotify(TNotifyType::ntInfo, "Calling init from wrapper ...");
	JsGateway->Gateway.Init();

	Args.GetReturnValue().Set(v8::Undefined(Isolate));
}

void TNodeJsEoGateway::startLearningMode(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	TNodeJsEoGateway* JsGateway = ObjectWrap::Unwrap<TNodeJsEoGateway>(Args.Holder());

	JsGateway->Notify->OnNotifyFmt(TNotifyType::ntInfo, "Starting learning mode from wrapper ...");

	JsGateway->Gateway.StartLearningMode();

	Args.GetReturnValue().Set(v8::Undefined(Isolate));
}

void TNodeJsEoGateway::on(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	TNodeJsEoGateway* JsGateway = ObjectWrap::Unwrap<TNodeJsEoGateway>(Args.Holder());

	const TStr EventId = TNodeJsUtil::GetArgStr(Args, 0);
	v8::Local<v8::Function> Cb = TNodeJsUtil::GetArgFun(Args, 1);

	if (EventId == "device") {
		JsGateway->OnDeviceCallback.Reset(Isolate, Cb);
	} else {
		throw TExcept::New("EventId: " + EventId + " not supported!");
	}


	Args.GetReturnValue().Set(v8::Undefined(Isolate));
}

void TNodeJsEoGateway::OnDeviceConnected(const uint32& DeviceId, const uchar& ROrg,
		const uchar& Func, const uchar& Type) {
	Notify->OnNotifyFmt(ntInfo, "Device connected in wrapper, ID %u ...", DeviceId);

	{
		TLock Lock(CallbackSection);
		NewDeviceIdQ.Add(TQuad<TUInt, TUCh, TUCh, TUCh>(DeviceId, ROrg, Func, Type));
	}

	TNodeJsAsyncUtil::ExecuteOnMain(new TProcessQueuesTask(this), CallbackHandle, true);
}

void TNodeJsEoGateway::OnMsg(const uint32& DeviceId, const eoMessage& Msg) {
	Notify->OnNotifyFmt(ntInfo, "Received EnOcean message from device %u, pushing to main thread ...", DeviceId);

	{
		TLock Lock(CallbackSection);
		DeviceIdMsgPrQ.Add(TPair<TUInt, eoMessage>(DeviceId, Msg));
	}

	TNodeJsAsyncUtil::ExecuteOnMain(new TProcessQueuesTask(this), CallbackHandle, true);
}

void TNodeJsEoGateway::AddDevice(const uint32& DeviceId, v8::Local<v8::Object>& JsDevice) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	EAssert(!DeviceMap.IsEmpty());

	v8::Local<v8::Object> Map = v8::Local<v8::Object>::New(Isolate, DeviceMap);
	Map->Set(v8::Integer::New(Isolate, DeviceId), JsDevice);
}

void TNodeJsEoGateway::ProcessQueues() {
	TVec<TQuad<TUInt, TUCh, TUCh, TUCh>> TempDevIdRorgFuncTypeQuV;
	TUIntV TempNewDeviceIdQ;
	TVec<TPair<TUInt, eoMessage>> TempDeviceIdMsgPrQ;

	{
		TLock Lock(CallbackSection);

		TempDevIdRorgFuncTypeQuV = NewDeviceIdQ;
		TempDeviceIdMsgPrQ = DeviceIdMsgPrQ;

		NewDeviceIdQ.Clr();
		DeviceIdMsgPrQ.Clr();
	}

	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	if (!TempDevIdRorgFuncTypeQuV.Empty() && !OnDeviceCallback.IsEmpty()) {
		v8::Local<v8::Function> Callback = v8::Local<v8::Function>::New(Isolate, OnDeviceCallback);

		for (int DeviceN = 0; DeviceN < TempDevIdRorgFuncTypeQuV.Len(); DeviceN++) {
			const TQuad<TUInt, TUCh, TUCh, TUCh>& TempDevIdRorgFuncTypeQu = TempDevIdRorgFuncTypeQuV[DeviceN];

			const uint& DeviceId = TempDevIdRorgFuncTypeQu.Val1;
			const uchar& ROrg = TempDevIdRorgFuncTypeQu.Val2;
			const uchar& Func = TempDevIdRorgFuncTypeQu.Val3;
			const uchar& Type = TempDevIdRorgFuncTypeQu.Val4;

			Notify->OnNotifyFmt(ntInfo, "Creating new JS device with ID %u ...", DeviceId);

			// add the device to the internal structures
			v8::Local<v8::Object> JsDevice = CreateNewDevice(ROrg, Func, Type);
			AddDevice(DeviceId, JsDevice);

			// execute callback
			TNodeJsUtil::ExecuteVoid(Callback, JsDevice);
		}
	}

	if (!TempDeviceIdMsgPrQ.Empty()) {
		for (int MsgN = 0; MsgN < TempDeviceIdMsgPrQ.Len(); MsgN++) {
			const TPair<TUInt, eoMessage>& DeviceIdMsgPr = TempDeviceIdMsgPrQ[MsgN];
			const uint& DeviceId = DeviceIdMsgPr.Val1;
			const eoMessage& Msg = DeviceIdMsgPr.Val2;

			Notify->OnNotifyFmt(ntInfo, "Received EnOcean message from device %u on main thread ...", DeviceId);
			TNodeJsEoDevice* Device = GetDevice(DeviceId);
			Device->OnMsg(Msg);
		}
	}
}

TNodeJsEoDevice* TNodeJsEoGateway::GetDevice(const uint32& DeviceId) const {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	v8::Local<v8::Number> Key = v8::Number::New(Isolate, DeviceId);
	v8::Local<v8::Object> Map = v8::Local<v8::Object>::New(Isolate, DeviceMap);

	Notify->OnNotifyFmt(ntInfo, "Fetching device %d\n", DeviceId);

	EAssertR(Map->Has(Key), "Device " + TUInt::GetStr(DeviceId) + " missing!");

	v8::Local<v8::Object> JsDevice = Map->Get(Key)->ToObject();
	return ObjectWrap::Unwrap<TNodeJsEoDevice>(JsDevice);
}

v8::Local<v8::Object> TNodeJsEoGateway::CreateNewDevice(const uchar& ROrg,
		const uchar& Func, const uchar& Type) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::EscapableHandleScope HandleScope(Isolate);

	switch (ROrg) {
	case RORG_RPS: {
		switch (Func) {
		case 0x02:
			return HandleScope.Escape(TNodeJsUtil::NewInstance(TNodeJsF602Rocker::New(DeviceId, &Gateway, Notify)));
		default:
			throw TExcept::New("Invalid profile combination: RORG: " + TInt::GetStr(ROrg) + ", Func: " + TInt::GetStr(Func));
		}
		break;
	}
	case RORG_VLD: {
		switch (Func) {
		case 0x01: {
			return HandleScope.Escape(TNodeJsUtil::NewInstance(TNodeJsD201Device::New(DeviceId, &Gateway, Notify)));
		}
		default:
			throw TExcept::New("Invalid profile combination: RORG: " + TInt::GetStr(ROrg) + ", Func: " + TInt::GetStr(Func));
		}
		break;
	}
	default: {
		throw TExcept::New("Invalid RORG: " + TInt::GetStr(ROrg));
	}
	}
}

TNodeJsEoGateway::TProcessQueuesTask::TProcessQueuesTask(TNodeJsEoGateway* _JsGateway):
			JsGateway(_JsGateway) {}

void TNodeJsEoGateway::TProcessQueuesTask::Run() {
	try {
		JsGateway->ProcessQueues();
	} catch (const PExcept& Except) {
		JsGateway->Notify->OnNotifyFmt(ntErr, "Exception while processing queues!");
	}
}


//////////////////////////////////////////////
// module initialization
void Init(v8::Handle<v8::Object> Exports) {
	TNodeJsRpiBase::Init(Exports);
	TNodejsDHT11Sensor::Init(Exports);
	TNodeJsYL40Adc::Init(Exports);
	TNodeJsRf24Radio::Init(Exports);

	// EnOcean
	TNodeJsD201Device::Init(Exports);
	TNodeJsEoGateway::Init(Exports);
}

NODE_MODULE(rpinode, Init);
