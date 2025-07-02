#include <node.h>

#include <iostream>

#include "isolate_handle/isolate_handle.h"
#include "platform_delegate.h"
#include "utils/script_wrappable.h"
#include "utils/utils.h"

namespace svm {

class Addon : public ScriptWrappable {
 public:
  explicit Addon(int a) : a_(a) { std::cout << "Addon()" << std::endl; }
  ~Addon() override { std::cout << "~Addon()" << std::endl; }

 private:
  int a_;
};

void AddonCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  Addon* addon = MakeCppGcObject<Addon>(info.GetIsolate(), 1);
  ScriptWrappable::Wrap(info.This(), addon);
}

void ReleaseCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  info.GetIsolate()->LowMemoryNotification();
}

void Initialize(v8::Local<v8::Object> exports) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::Local<v8::FunctionTemplate> tpl =
      v8::FunctionTemplate::New(isolate, AddonCallback);
  tpl->InstanceTemplate()->SetInternalFieldCount(2);
  exports->Set(context, toString("test"),
               tpl->GetFunction(context).ToLocalChecked());

  v8::Local<v8::FunctionTemplate> tpl1 =
      v8::FunctionTemplate::New(isolate, ReleaseCallback);
  exports->Set(context, toString("gc"),
               tpl1->GetFunction(context).ToLocalChecked());

  exports->Set(
      context, toString("Isolate"),
      IsolateHandle::Interface(isolate)->GetFunction(context).ToLocalChecked());

  PlatformDelegate::InitializeDelegate();
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)
}  // namespace svm
