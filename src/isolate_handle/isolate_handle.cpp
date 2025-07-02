#include "isolate_handle.h"

#include <iostream>

#include "../utils/utils.h"

namespace svm {

void ConstructCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (info.IsConstructCall()) {
    ScriptWrappable::Wrap(info.This(), IsolateHandle::Create());
    return;
  }

  info.GetIsolate()->ThrowException(v8::Exception::TypeError(
      toString(info.GetIsolate(), "Illegal constructor")));
}

void AttributeGetterTestCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  info.GetReturnValue().Set(9999);
}

IsolateHandle::IsolateHandle() {
  std::cout << "IsolateHandle()" << std::endl;
}
IsolateHandle::~IsolateHandle() {
  std::cout << "~IsolateHandle()" << std::endl;
}

v8::Local<v8::FunctionTemplate> IsolateHandle::Interface(v8::Isolate* isolate) {
  ConstructItem constructor{"Isolate", 0, ConstructCallback};
  AttributeItem attrs[]{
      {"value", AttributeGetterTestCallback, nullptr,
       v8::PropertyAttribute::ReadOnly, Dependence::kPrototype},
  };
  // OperationItem operas[]{};

  v8::Local<v8::FunctionTemplate> interface =
      InstallConstructor(isolate, constructor);
  v8::Local<v8::Signature> signature = v8::Signature::New(isolate, interface);
  InstallAttributes(isolate, interface, attrs, signature);
  // InstallOperations(isolate, interface, operas, signature);

  return interface;
}

IsolateHandle* IsolateHandle::Create() {
  return MakeCppGcObject<IsolateHandle>();
}

}  // namespace svm
