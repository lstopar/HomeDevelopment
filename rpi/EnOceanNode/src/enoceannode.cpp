#include "enoceannode.h"

eoDevice* TNodeJsEoDevice::GetDevice() const {
	eoDevice* Device = Gateway->GetDevice(DeviceId);
	EAssertR(Device != nullptr, "Device missing in gateway!");
	return Device;
}

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

	const uint8 ChannelN = (uint8) TNodeJsUtil::GetArgInt32(Args, 0);
	const uint8 Value = (uint8) TNodeJsUtil::GetArgInt32(Args, 1);

	TEoGateway* Gateway = JsDevice->GetGateway();

	eoMessage Msg;
	eoReturn Code = eoEEP_D201xx::CreateSetOutput(
			Gateway->GetId(),
			JsDevice->DeviceId,
			0x00,
			ChannelN,
			Value,
			Msg
	);

	EAssertR(Code == EO_OK, "Failed to generate message!");

	Gateway->Send(Msg);

	Args.GetReturnValue().Set(v8::Undefined(Isolate));
}

void TNodeJsD201Device::readStatus(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);
	// unwrap
	TNodeJsD201Device* JsDevice = ObjectWrap::Unwrap<TNodeJsD201Device>(Args.Holder());

	const int ChannelN = TNodeJsUtil::GetArgInt32(Args, 0, -1);

	const uint8 Channel = ChannelN >= 0 ? (uint8) ChannelN : 0x1E;

	TEoGateway* Gateway = JsDevice->GetGateway();

	eoMessage Msg;
	eoReturn Code = eoEEP_D201xx::CreateStatusQuery(
			Gateway->GetId(),
			JsDevice->DeviceId,
			Channel,
			Msg
	);

	EAssertR(Code == EO_OK, "Failed to generate message!");

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
	Info.GetReturnValue().Set(v8::Integer::New(Isolate, JsDevice->DeviceId));
}

void TNodeJsD201Device::type(v8::Local<v8::String> Name, const v8::PropertyCallbackInfo<v8::Value>& Info) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	Info.GetReturnValue().Set(v8::String::NewFromUtf8(Isolate, "D2-01-xx"));
}

void TNodeJsD201Device::OnMsg(const eoMessage& Msg) {
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
}

void TNodeJsD201Device::OnStatus(const uint8& Channel, const uint8& Val) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	if (!StatusCb.IsEmpty()) {
		v8::Local<v8::Function> Callback = v8::Local<v8::Function>::New(Isolate, StatusCb);
		TNodeJsUtil::ExecuteVoid(Callback, v8::Integer::New(Isolate, (int) Channel), v8::Integer::New(Isolate, (int) Val));
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

void TNodeJsEoGateway::OnDeviceConnected(const uint32& DeviceId) {
	Notify->OnNotifyFmt(ntInfo, "Device connected in wrapper, ID %u ...", DeviceId);
	TNodeJsAsyncUtil::ExecuteOnMain(new TOnDeviceConnectedTask(this, DeviceId), true);
}

void TNodeJsEoGateway::OnMsg(const uint32& DeviceId, const eoMessage& Msg) {
	TNodeJsAsyncUtil::ExecuteOnMainAndWait(new TOnMsgTask(this, DeviceId, Msg), true);
}

void TNodeJsEoGateway::AddDevice(const uint32& DeviceId, v8::Local<v8::Object>& JsDevice) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	EAssert(!DeviceMap.IsEmpty());

	v8::Local<v8::Object> Map = v8::Local<v8::Object>::New(Isolate, DeviceMap);
	Map->Set(v8::Integer::New(Isolate, DeviceId), JsDevice);
}

void TNodeJsEoGateway::OnMsgMainThread(const uint32& DeviceId, const eoMessage& Msg) {
	TNodeJsEoDevice* Device = GetDevice(DeviceId);
	Device->OnMsg(Msg);
}

TNodeJsEoDevice* TNodeJsEoGateway::GetDevice(const uint32& DeviceId) const {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	v8::Local<v8::Integer> Key = v8::Integer::New(Isolate, DeviceId);
	v8::Local<v8::Object> Map = v8::Local<v8::Object>::New(Isolate, DeviceMap);

	EAssertR(Map->Has(Key), "Device " + TUInt::GetStr(DeviceId) + " missing!");

	v8::Local<v8::Object> JsDevice = Map->Get(Key)->ToObject();
	return ObjectWrap::Unwrap<TNodeJsEoDevice>(JsDevice);
}

TNodeJsEoGateway::TOnDeviceConnectedTask::TOnDeviceConnectedTask(TNodeJsEoGateway* _JsGateway,
		const uint32& _DeviceId):
			JsGateway(_JsGateway),
			DeviceId(_DeviceId) {}

void TNodeJsEoGateway::TOnDeviceConnectedTask::Run() {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	// add the device to the internal structures

	JsGateway->Notify->OnNotifyFmt(ntInfo, "Creating new JS device with ID %u ...", DeviceId);

	// TODO check which type of device this is
	v8::Local<v8::Object> JsDevice = TNodeJsUtil::NewInstance(TNodeJsD201Device::New(DeviceId, &JsGateway->Gateway));

	JsGateway->AddDevice(DeviceId, JsDevice);

	if (!JsGateway->OnDeviceCallback.IsEmpty()) {
		v8::Local<v8::Function> Callback = v8::Local<v8::Function>::New(Isolate, JsGateway->OnDeviceCallback);
		TNodeJsUtil::ExecuteVoid(Callback, JsDevice);
	}
}

TNodeJsEoGateway::TOnMsgTask::TOnMsgTask(TNodeJsEoGateway* _Gateway,
		const uint32& _DeviceId, const eoMessage& _Msg):
		Gateway(_Gateway),
		DeviceId(_DeviceId),
		Msg(_Msg) {}

void TNodeJsEoGateway::TOnMsgTask::Run() {
	Gateway->OnMsgMainThread(DeviceId, Msg);
}

//////////////////////////////////////////////
// module initialization
void Init(v8::Handle<v8::Object> Exports) {
	TNodeJsD201Device::Init(Exports);
	TNodeJsEoGateway::Init(Exports);
}

NODE_MODULE(enoceannode, Init);
