#include <node.h>

#include <iostream>

#include "isolate/platform_delegate.h"
#include "module/isolate_handle.h"
#include "utils/script_wrappable.h"
#include "utils/utils.h"

namespace svm {
class Test {

  int a = 0;
};

class Addon1 final : public ScriptWrappable {
 public:
  explicit Addon1(int a) : a_(a) { std::cout << "Addon1()" << std::endl; }
  ~Addon1() override { std::cout << "~Addon1()" << std::endl; }

  void Trace(cppgc::Visitor* visitor) const override {
    ScriptWrappable::Trace(visitor);
  }

 private:
  int a_;
};

class Addon final : public ScriptWrappable {
 public:
  explicit Addon(int a) : a_(a) {
    std::cout << "Addon()" << std::endl;
    addon_ = MakeCppGcObject<Addon1>(v8::Isolate::GetCurrent(), 1000);
  }
  ~Addon() override { std::cout << "~Addon()" << std::endl; }

  void Trace(cppgc::Visitor* visitor) const override {
    visitor->Trace(addon_);
    ScriptWrappable::Trace(visitor);
  }

 private:
  int a_;
  std::unique_ptr<Test> test;
  cppgc::Member<Addon1> addon_;
};

void AddonCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  Addon* addon = MakeCppGcObject<Addon>(info.GetIsolate(), 1);
  ScriptWrappable::Wrap(info.This(), addon);
}

void GcCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
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
      v8::FunctionTemplate::New(isolate, GcCallback);
  exports->Set(context, toString("gc"),
               tpl1->GetFunction(context).ToLocalChecked());

  exports->Set(
      context, toString("Isolate"),
      IsolateHandle::Interface(isolate)->GetFunction(context).ToLocalChecked());

  PlatformDelegate::InitializeDelegate();
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)
}  // namespace svm
