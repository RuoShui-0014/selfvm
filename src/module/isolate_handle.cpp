#include "isolate_handle.h"

#include <cppgc/allocation.h>
#include <cppgc/internal/write-barrier.h>
#include <cppgc/member.h>
#include <cppgc/visitor.h>

#include <iostream>

#include "../utils/utils.h"
#include "context_handle.h"


namespace svm {

void ConstructCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (info.IsConstructCall()) {
    ScriptWrappable::Wrap(info.This(),
                          IsolateHandle::Create(info.GetIsolate()));
    return;
  }

  info.GetIsolate()->ThrowException(v8::Exception::TypeError(
      toString(info.GetIsolate(), "Illegal constructor")));
}

void AttributeGetterContextCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> reciver = info.This();
  IsolateHandle* isolate_handle =
      ScriptWrappable::Unwrap<IsolateHandle>(reciver);
  ContextHandle* context_handle = isolate_handle->GetContextHandle();
  info.GetReturnValue().Set(context_handle->V8Object(info.GetIsolate()));
}

IsolateHandle::IsolateHandle(std::unique_ptr<IsolateHolder>& isolate_holder)
    : isolate_holder_(std::move(isolate_holder)) {
  std::cout << "IsolateHandle()" << std::endl;
}

IsolateHandle::~IsolateHandle() {
  std::cout << "~IsolateHandle()" << std::endl;
}

ContextHandle* IsolateHandle::GetContextHandle() {
  if (!default_context_) {
    default_context_ = ContextHandle::Create(this);
  }
  return default_context_.Get();
}

void IsolateHandle::Trace(cppgc::Visitor* visitor) const {
  visitor->Trace(default_context_);
  ScriptWrappable::Trace(visitor);
}

v8::Local<v8::FunctionTemplate> IsolateHandle::Interface(v8::Isolate* isolate) {
  ConstructItem constructor{"Isolate", 0, ConstructCallback};
  AttributeItem attrs[]{
      {"context", AttributeGetterContextCallback, nullptr,
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

IsolateHandle* IsolateHandle::Create(v8::Isolate* isolate) {
  size_t memory_limit = 128;

  auto isolate_holder = std::make_unique<IsolateHolder>(isolate, memory_limit);

  return MakeCppGcObject<IsolateHandle>(isolate, isolate_holder);
}

}  // namespace svm
