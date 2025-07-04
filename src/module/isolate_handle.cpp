#include "isolate_handle.h"

#include <cppgc/allocation.h>
#include <cppgc/member.h>
#include <cppgc/visitor.h>

#include <iostream>

#include "../utils/utils.h"
#include "context_handle.h"

namespace svm {

IsolateHandle* IsolateHandle::Create(v8::Isolate* isolate) {
  size_t memory_limit = 128;

  auto isolate_holder = std::make_unique<IsolateHolder>(isolate, memory_limit);

  return MakeCppGcObject<GC::kSpecified, IsolateHandle>(isolate,
                                                        isolate_holder);
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

void IsolateHandle::Release() {
  isolate_holder_.reset();
}

void IsolateHandle::Trace(cppgc::Visitor* visitor) const {
  visitor->Trace(default_context_);
  ScriptWrappable::Trace(visitor);
}

/*************************************************************************************************/

void ConstructCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (info.IsConstructCall()) {
    ScriptWrappable::Wrap(info.This(),
                          IsolateHandle::Create(info.GetIsolate()));
    return;
  }

  info.GetIsolate()->ThrowException(v8::Exception::TypeError(
      toString(info.GetIsolate(), "Illegal constructor")));
}

void ContextAttributeGetCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> receiver = info.This();
  IsolateHandle* isolate_handle =
      ScriptWrappable::Unwrap<IsolateHandle>(receiver);
  ContextHandle* context_handle = isolate_handle->GetContextHandle();
  info.GetReturnValue().Set(context_handle->V8Object(info.GetIsolate()));
}

void ReleaseOperationCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> receiver = info.This();
  IsolateHandle* isolate_handle =
      ScriptWrappable::Unwrap<IsolateHandle>(receiver);
  isolate_handle->Release();
}

void V8IsolateHandle::InstallInterfaceTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::Template> interface_template) {
  ConstructItem constructor{"Isolate", 0, ConstructCallback};
  AttributeItem attrs[]{
      {"context", ContextAttributeGetCallback, nullptr,
       v8::PropertyAttribute::ReadOnly, Dependence::kPrototype},
  };
  OperationItem operas[]{
      {"release", 0, ReleaseOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
  };

  InstallConstructor(isolate, interface_template, constructor);

  v8::Local<v8::Signature> signature =
      v8::Local<v8::Signature>::Cast(interface_template);
  InstallAttributes(isolate, interface_template, attrs, signature);
  InstallOperations(isolate, interface_template, operas, signature);
}

const WrapperTypeInfo V8IsolateHandle::wrapper_type_info_{
    "Isolate", V8IsolateHandle::InstallInterfaceTemplate, nullptr};

}  // namespace svm
