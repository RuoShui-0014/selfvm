#include "context_handle.h"

#include <iostream>

#include "../isolate/external_data.h"
#include "../isolate/per_isolate_data.h"
#include "../utils/utils.h"
#include "isolate_handle.h"

namespace svm {

ContextHandle* ContextHandle::Create(IsolateHandle* isolate_handle) {
  v8::Local<v8::Context> context =
      isolate_handle->GetIsolateHolder()->NewContext();
  v8::Isolate* isolate = isolate_handle->GetParentIsolate();
  ContextHandle* context_handle =
      MakeCppGcObject<GC::kSpecified, ContextHandle>(isolate, isolate_handle,
                                                     context);
  v8::Local<v8::FunctionTemplate> isolate_handle_template =
      V8ContextHandle::GetWrapperTypeInfo()
          ->GetV8ClassTemplate(isolate)
          .As<v8::FunctionTemplate>();
  v8::Local<v8::Object> obj = isolate_handle_template->InstanceTemplate()
                                  ->NewInstance(isolate->GetCurrentContext())
                                  .ToLocalChecked();
  ScriptWrappable::Wrap(obj, context_handle);
  return context_handle;
}

ContextHandle::ContextHandle(IsolateHandle* isolate_handle,
                             v8::Local<v8::Context> context)
    : isolate_handle_(isolate_handle) {
  context_.Reset(isolate_handle->GetIsolate(), context);
  std::cout << "ContextHandle()" << std::endl;
}

ContextHandle::~ContextHandle() {
  context_.Clear();
  std::cout << "~ContextHandle()" << std::endl;
}

void ContextHandle::Eval(std::string script,
                         v8::Local<v8::Value>* result) const {
  v8::Isolate* self_isolate = isolate_handle_->GetIsolate();
  v8::Isolate* parent_isolate = isolate_handle_->GetParentIsolate();

  v8::Locker self_locker(self_isolate);
  V8Scope self_scope(self_isolate, &context_);

  ScriptTask task(self_isolate, self_scope.GetContext(), script);
  task.Run();
  ExternalData::Data source_data = {self_isolate, self_scope.GetContext(),
                                    task.GetResult()};

  {
    v8::Locker parent_locker(parent_isolate);
    V8Scope parent_scope(parent_isolate, parent_isolate->GetCurrentContext());

    ExternalData::Data target_data = {parent_isolate,
                                      parent_scope.GetContext()};
    if (ExternalData::Copy(target_data, source_data)) {
      *result = target_data.value;
    }
  }
}

void ContextHandle::Release() {
  context_.Clear();
}

void ContextHandle::Trace(cppgc::Visitor* visitor) const {
  visitor->Trace(isolate_handle_);
  ScriptWrappable::Trace(visitor);
}

///////////////////////////////////////////////////////////////////////////////

void EvalOperationCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (info.Length() < 1 || !info[0]->IsString()) {
    return;
  }

  v8::Local<v8::Object> receiver = info.This();
  ContextHandle* context_handle =
      ScriptWrappable::Unwrap<ContextHandle>(receiver);
  v8::Local<v8::Value> result = v8::Undefined(info.GetIsolate());
  context_handle->Eval(*v8::String::Utf8Value(info.GetIsolate(), info[0]),
                       &result);

  info.GetReturnValue().Set(result);
}

void V8ContextHandle::InstallInterfaceTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::Template> interface_template) {
  ConstructItem constructor{"Context", 0, nullptr};
  // AttributeItem attrs[]{
  //     {"context", AttributeGetterContextCallback, nullptr,
  //      v8::PropertyAttribute::ReadOnly, Dependence::kPrototype},
  // };
  OperationItem operas[]{
      {"eval", 2, EvalOperationCallback, v8::PropertyAttribute::DontDelete,
       Dependence::kPrototype},
  };

  InstallConstructor(isolate, interface_template, constructor);

  v8::Local<v8::Signature> signature =
      v8::Local<v8::Signature>::Cast(interface_template);
  // InstallAttributes(isolate, interface_template, attrs, signature);
  InstallOperations(isolate, interface_template, operas, signature);
}

const WrapperTypeInfo V8ContextHandle::wrapper_type_info_{
    "Context", V8ContextHandle::InstallInterfaceTemplate, nullptr};

}  // namespace svm
