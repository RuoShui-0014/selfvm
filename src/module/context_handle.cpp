#include "context_handle.h"

#include "../isolate/per_isolate_data.h"
#include "../isolate/task.h"
#include "../utils/utils.h"
#include "isolate_handle.h"

namespace svm {

ContextHandle* ContextHandle::Create(IsolateHandle* isolate_handle) {
  auto task = std::make_unique<CreateContextTask>(isolate_handle);
  auto waiter = task->CreateWaiter();
  isolate_handle->PostTask(std::move(task));

  auto id = waiter->GetResult();

  v8::Isolate* isolate = isolate_handle->GetParentIsolate();
  ContextHandle* context_handle =
      MakeCppGcObject<GC::kSpecified, ContextHandle>(isolate, isolate_handle,
                                                     id);
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

ContextHandle::ContextHandle(IsolateHandle* isolate_handle, uint32_t context_id)
    : isolate_(isolate_handle->GetIsolate()),
      id_{context_id},
      isolate_handle_(isolate_handle) {}

ContextHandle::~ContextHandle() = default;

// 同步任务
std::pair<uint8_t*, size_t> ContextHandle::Eval(std::string script) {
  auto task = std::make_unique<ScriptTask>(this, script);
  auto waiter = task->CreateWaiter();
  PostTask(std::move(task));
  return waiter->GetResult();
}

void ContextHandle::EvalAsync(std::unique_ptr<AsyncInfo> info,
                              std::string script) {
  auto task = std::make_unique<ScriptAsyncTask>(std::move(info), this, script);
  PostTask(std::move(task));
}

void ContextHandle::Release() {
  isolate_handle_->GetIsolateHolder()->ClearContext(id_);
}

v8::Local<v8::Context> ContextHandle::GetContext() {
  return isolate_handle_->GetContext(id_);
}

void ContextHandle::PostTask(std::unique_ptr<v8::Task> task) {
  isolate_handle_->PostTask(std::move(task));
}

void ContextHandle::PostTaskToParent(std::unique_ptr<v8::Task> task) {
  isolate_handle_->PostTaskToParent(std::move(task));
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

  v8::Isolate* isolate = info.GetIsolate();
  v8::HandleScope scope(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Object> receiver = info.This();
  ContextHandle* context_handle =
      ScriptWrappable::Unwrap<ContextHandle>(receiver);
  auto buff = context_handle->Eval(*v8::String::Utf8Value(isolate, info[0]));

  v8::Local<v8::Value> result, error;
  {
    v8::TryCatch try_catch(isolate);
    v8::ValueDeserializer deserializer(isolate, buff.first, buff.second);
    deserializer.ReadHeader(context);
    if (deserializer.ReadValue(context).ToLocal(&result)) {
      if (result->IsNativeError()) {
        error = result;
      }
    } else {
      error = try_catch.Exception();
      try_catch.Reset();
    }
  }

  if (error.IsEmpty()) {
    info.GetReturnValue().Set(result);
  } else {
    isolate->ThrowException(error);
  }
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

  auto async_info = std::make_unique<AsyncInfo>(
      context_handle->GetIsolateHandle(), isolate,
      RemoteHandle(isolate, isolate->GetCurrentContext()),
      RemoteHandle(isolate, resolver));
  context_handle->EvalAsync(std::move(async_info),
                            *v8::String::Utf8Value(isolate, info[0]));

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
      {"eval", 2, EvalOperationCallback, v8::PropertyAttribute::DontDelete,
       Dependence::kPrototype},
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
