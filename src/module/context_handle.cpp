#include "context_handle.h"

#include <iostream>

#include "../isolate/per_isolate_data.h"
#include "../isolate/task.h"
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
    : context_(isolate_handle->GetIsolate(), context),
      isolate_handle_(isolate_handle) {}

ContextHandle::~ContextHandle() = default;

// 同步任务
void ContextHandle::Eval(std::string script, v8::Local<v8::Value>* result) {
  v8::Isolate* isolate_parent = isolate_handle_->GetParentIsolate();
  v8::HandleScope result_scope(isolate_parent);
  ScriptTask::ResultInfo result_info{
    isolate_parent, isolate_parent->GetCurrentContext(), result};

  {
    v8::Isolate* isolate = isolate_handle_->GetIsolate();
    v8::Locker locker(isolate);
    v8::HandleScope scope(isolate);

    ScriptTask::ScriptInfo script_info{isolate, context_.Get(isolate),
                                   std::move(script)};
    ScriptTask task(script_info, result_info);
    task.Run();
  }
}

void ContextHandle::EvalAsync(std::string script,
                              v8::Local<v8::Promise::Resolver> resolver) {
  v8::Isolate* isolate_parent = isolate_handle_->GetParentIsolate();
  v8::HandleScope scope_parent(isolate_parent);

  Scheduler* scheduler_self =
      isolate_handle_->GetIsolateHolder()->GetScheduler();
  Scheduler* scheduler_parent =
      isolate_handle_->GetIsolateHolder()->GetParentScheduler();

  AsyncTask::AsyncInfo async_info{scheduler_parent, isolate_parent,
                                  isolate_parent->GetCurrentContext(),
                                  resolver};

  {
    v8::Isolate* isolate_self = isolate_handle_->GetIsolate();
    v8::Locker locker_self(isolate_self);
    v8::HandleScope scope_self(isolate_self);

    ScriptTaskAsync::ScriptInfo script_info{
        isolate_self, context_.Get(isolate_self), std::move(script)};

    auto task = std::make_unique<ScriptTaskAsync>(async_info, script_info);
    scheduler_self->TaskRunner()->PostTask(std::move(task));
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
  context_handle->Eval(*v8::String::Utf8Value(isolate, info[0]), &result);

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

  info.GetReturnValue().Set(resolver);
}

void V8ContextHandle::InstallInterfaceTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::Template> interface_template) {
  ConstructConfig constructor{"Context", 0, nullptr};
  // AttributeConfig attrs[]{
  //     {"context", AttributeGetterContextCallback, nullptr,
  //      v8::PropertyAttribute::ReadOnly, Dependence::kPrototype},
  // };
  OperationConfig operas[]{
      {"eval", 2, EvalSyncOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
      {"evalAsync", 2, EvalAsyncOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
  };

  InstallConstructor(isolate, interface_template, constructor);

  v8::Local<v8::Signature> signature =
      v8::Local<v8::Signature>::Cast(interface_template);
  // InstallAttributes(isolate, interface_template, signature, attrs);
  InstallOperations(isolate, interface_template, signature, operas);
}

const WrapperTypeInfo V8ContextHandle::wrapper_type_info_{
    "Context", V8ContextHandle::InstallInterfaceTemplate, nullptr};

}  // namespace svm
