#include "context_handle.h"

#include <iostream>

#include "../isolate/per_isolate_data.h"
#include "../isolate/task.h"
#include "../utils/utils.h"
#include "async_manager.h"
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

// 同步任务
void ContextHandle::EvalSync(std::string script,
                             v8::Local<v8::Value>* result) const {
  v8::Isolate* isolate = isolate_handle_->GetIsolate();
  v8::Locker locker(isolate);
  v8::HandleScope scope(isolate);

  ScriptTask::CallInfo call_info{isolate, context_.Get(isolate),
                                 std::move(script)};

  {
    v8::Isolate* result_isolate = isolate_handle_->GetParentIsolate();
    v8::Locker result_locker(isolate);
    v8::HandleScope result_scope(isolate);
    ScriptTask::ResultInfo result_info{
        result_isolate, result_isolate->GetCurrentContext(), result};
    ScriptTask task(call_info, result_info);
    task.Run();
  }
}

void ContextHandle::EvalAsync(std::string script,
                              v8::Local<v8::Promise::Resolver> resolver) const {
  v8::Isolate* isolate = isolate_handle_->GetIsolate();
  v8::Locker locker(isolate);
  v8::HandleScope scope(isolate);

  ScriptTaskAsync::CallInfo call_info{
      isolate, {isolate, context_.Get(isolate)}, std::move(script)};

  {
    v8::Isolate* result_isolate = isolate_handle_->GetParentIsolate();
    v8::Locker result_locker(isolate);
    AsyncManager* async_manager = isolate_handle_->GetAsyncManager();
    ScriptTaskAsync::ResultInfo result_info{
        async_manager,
        result_isolate,
        {isolate, result_isolate->GetCurrentContext()},
        {isolate, resolver}};
    ScriptTaskAsync task(call_info, result_info);
    task.Run();
    async_manager->Send();
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

void EvalSyncOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (info.Length() < 1 || !info[0]->IsString()) {
    return;
  }

  v8::Isolate* isolate = info.GetIsolate();
  v8::HandleScope scope(isolate);
  v8::Local<v8::Object> receiver = info.This();
  ContextHandle* context_handle =
      ScriptWrappable::Unwrap<ContextHandle>(receiver);
  v8::Local<v8::Value> result = v8::Undefined(isolate);
  context_handle->EvalSync(*v8::String::Utf8Value(isolate, info[0]), &result);

  info.GetReturnValue().Set(result);
}

void EvalAsyncOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (info.Length() < 1 || !info[0]->IsString()) {
    return;
  }

  v8::Isolate* isolate = info.GetIsolate();
  v8::HandleScope scope(isolate);
  v8::Local<v8::Object> receiver = info.This();
  ContextHandle* context_handle =
      ScriptWrappable::Unwrap<ContextHandle>(receiver);
  v8::Local<v8::Promise::Resolver> resolver =
      v8::Promise::Resolver::New(isolate->GetCurrentContext()).ToLocalChecked();
  context_handle->EvalAsync(*v8::String::Utf8Value(isolate, info[0]), resolver);

  info.GetReturnValue().Set(resolver->GetPromise());
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
      {"evalSync", 2, EvalSyncOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
      {"evalAsync", 2, EvalAsyncOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
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
