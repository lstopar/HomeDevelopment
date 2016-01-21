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

	const int Pin = TNodeJsUtil::GetArgInt32(Args, 0);
	return new TNodejsDHT11Sensor(new TDHT11Sensor(Pin));
}

TNodejsDHT11Sensor::TNodejsDHT11Sensor(TDHT11Sensor* _Sensor):
		Sensor(_Sensor) {}

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
		Sensor(nullptr) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	TNodejsDHT11Sensor* JsSensor = ObjectWrap::Unwrap<TNodejsDHT11Sensor>(Args.Holder());
	Sensor = JsSensor->Sensor;
}

v8::Handle<v8::Function> TNodejsDHT11Sensor::TReadTask::GetCallback(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	return TNodeJsUtil::GetArgFun(Args, 0);
}

v8::Local<v8::Value> TNodejsDHT11Sensor::TReadTask::WrapResult() {
	printf("called correct wrap result ...\n");

	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	const double Temp = Sensor->GetTemp();
	const double Hum = Sensor->GetHum();

	printf("Wrapping result: temp: %.3f, hum: %.3f\n", Temp, Hum);

	v8::Local<v8::Object> RetVal = v8::Object::New(Isolate);
	RetVal->Set(v8::String::NewFromUtf8(Isolate, "temperature"), v8::Number::New(Isolate, Temp));
	RetVal->Set(v8::String::NewFromUtf8(Isolate, "humidity"), v8::Number::New(Isolate, Hum));

	return RetVal;
}

void TNodejsDHT11Sensor::TReadTask::Run() {
	try {
		Sensor->ReadSensor();
	} catch (const PExcept& Except) {
		SetExcept(Except);
	}
}

//////////////////////////////////////////////
// module initialization
void Init(v8::Handle<v8::Object> Exports) {
	TNodejsDHT11Sensor::Init(Exports);
}

NODE_MODULE(rpinode, Init);
