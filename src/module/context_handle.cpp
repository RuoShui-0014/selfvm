#include "context_handle.h"

#include <cppgc/visitor.h>

#include <iostream>

#include "../isolate/external_data.h"
#include "../isolate/isolate_holder.h"
#include "../isolate/task.h"
#include "../utils/utils.h"
#include "isolate_handle.h"

namespace svm {

class ScriptTask final : public SyncFastTask<std::pair<uint8_t*, size_t>> {
 public:
  ScriptTask(ContextHandle* context_handle,
             std::string script,
             std::string filename)
      : context_handle_(context_handle),
        script_(std::move(script)),
        filename_(std::move(filename)) {}
  ~ScriptTask() override = default;

  void Run() override {
    v8::Isolate* isolate = context_handle_->GetIsolateSel();
    v8::HandleScope source_scope(isolate);
    v8::Local<v8::Context> context = context_handle_->GetContext();
    v8::Context::Scope context_scope{context};

    v8::TryCatch try_catch(isolate);
    v8::Local<v8::Script> script;
    v8::Local<v8::String> code = toString(isolate, script_);
    v8::ScriptOrigin scriptOrigin =
        v8::ScriptOrigin(toString(isolate, filename_));
    if (v8::Script::Compile(context, code, &scriptOrigin).ToLocal(&script)) {
      v8::MaybeLocal<v8::Value> maybe_result = script->Run(context);
      if (!maybe_result.IsEmpty()) {
        ExternalData::SourceData data{isolate, context,
                                      maybe_result.ToLocalChecked()};
        SetResult(ExternalData::SerializerSync(data));
        return;
      }
    }

    if (try_catch.HasCaught()) {
      ExternalData::SourceData data{isolate, context, try_catch.Exception()};
      SetResult(ExternalData::SerializerSync(data));
      try_catch.Reset();
    }
  }

 private:
  cppgc::Member<ContextHandle> context_handle_;
  std::string script_;
  std::string filename_;
};
class ScriptAsyncTask final : public AsyncTask {
 public:
  class Callback : public v8::Task {
   public:
    explicit Callback(std::unique_ptr<AsyncInfo> info,
                      std::pair<uint8_t*, size_t> buff)
        : info_(std::move(info)), buff_(std::move(buff)) {}
    ~Callback() override = default;

    void Run() override {
      v8::Isolate* isolate = info_->isolate;
      v8::HandleScope source_scope(isolate);
      v8::Local<v8::Context> context = info_->context.Get(isolate);
      v8::Context::Scope context_scope{context};

      v8::Local<v8::Value> result, error;
      {
        v8::TryCatch try_catch(isolate);
        v8::ValueDeserializer deserializer(isolate, buff_.first, buff_.second);
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
        info_->resolver.Get(isolate)->Resolve(context, result);
      } else {
        info_->resolver.Get(isolate)->Resolve(context, error);
      }
    }

   private:
    std::unique_ptr<AsyncInfo> info_;
    std::pair<uint8_t*, size_t> buff_;
  };
  explicit ScriptAsyncTask(std::unique_ptr<AsyncInfo> info,
                           ContextHandle* context_handle,
                           std::string script,
                           std::string filename)
      : AsyncTask(std::move(info)),
        context_handle_(context_handle),
        script_(std::move(script)),
        filename_(std::move(filename)) {}
  ~ScriptAsyncTask() override = default;

  void Run() override {
    v8::Isolate* isolate = context_handle_->GetIsolateSel();
    v8::HandleScope source_scope(isolate);
    v8::Local<v8::Context> context = context_handle_->GetContext();
    v8::Context::Scope context_scope{context};

    v8::TryCatch try_catch(isolate);
    v8::Local<v8::Script> script;
    v8::Local<v8::String> code = toString(isolate, script_);
    v8::ScriptOrigin scriptOrigin =
        v8::ScriptOrigin(toString(isolate, filename_));
    if (v8::Script::Compile(context, code, &scriptOrigin).ToLocal(&script)) {
      v8::MaybeLocal<v8::Value> maybe_result = script->Run(context);
      if (!maybe_result.IsEmpty()) {
        ExternalData::SourceData data{isolate, context,
                                      maybe_result.ToLocalChecked()};
        auto buff = ExternalData::SerializerAsync(data);
        IsolateHandle* isolate_handle = context_handle_->GetIsolateHandle();
        isolate_handle->PostHandleTaskToPar(
            std::make_unique<ScriptAsyncTask::Callback>(std::move(info_),
                                                        buff));
        return;
      }
    }

    if (try_catch.HasCaught()) {
      ExternalData::SourceData data{isolate, context, try_catch.Exception()};
      auto buff = ExternalData::SerializerAsync(data);
      IsolateHandle* isolate_handle = context_handle_->GetIsolateHandle();
      isolate_handle->PostHandleTaskToPar(
          std::make_unique<ScriptAsyncTask::Callback>(std::move(info_), buff));
      try_catch.Reset();
    }
  }

 private:
  cppgc::Member<ContextHandle> context_handle_;
  std::string script_;
  std::string filename_;
};

ContextHandle::ContextHandle(IsolateHandle* isolate_handle,
                             v8::Context* address)
    : isolate_handle_(isolate_handle),
      isolate_holder_(isolate_handle->GetIsolateHolder()),
      address_{address} {}

ContextHandle::~ContextHandle() {
  Release();
#ifdef DEBUG
  std::cout << "~ContextHandle()" << std::endl;
#endif
}

// 同步任务
std::pair<uint8_t*, size_t> ContextHandle::Eval(std::string script,
                                                std::string filename) {
  auto task = std::make_unique<ScriptTask>(this, std::move(script),
                                           std::move(filename));
  auto future = task->GetFuture();
  PostTaskToSel(std::move(task));
  return future->get();
}

void ContextHandle::EvalAsync(std::unique_ptr<AsyncInfo> info,
                              std::string script,
                              std::string filename) {
  auto task = std::make_unique<ScriptAsyncTask>(
      std::move(info), this, std::move(script), std::move(filename));
  PostTaskToSel(std::move(task));
}

void ContextHandle::Release() {
  isolate_holder_->ClearContext(address_);
}
v8::Isolate* ContextHandle::GetIsolateSel() {
  return isolate_holder_->GetIsolateSel();
}
v8::Isolate* ContextHandle::GetIsolatePar() {
  return isolate_holder_->GetIsolatePar();
}

v8::Local<v8::Context> ContextHandle::GetContext() {
  return isolate_holder_->GetContext(address_);
}

void ContextHandle::PostTaskToSel(std::unique_ptr<v8::Task> task) {
  isolate_holder_->PostTaskToSel(std::move(task));
}

void ContextHandle::PostTaskToPar(std::unique_ptr<v8::Task> task) {
  isolate_holder_->PostTaskToPar(std::move(task));
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

  std::string script = *v8::String::Utf8Value(isolate, info[0]);
  std::string filename{""};
  if (info.Length() > 1) {
    filename = *v8::String::Utf8Value(isolate, info[1]);
  }
  auto buff = context_handle->Eval(std::move(script), std::move(filename));

  v8::Local<v8::Value> result =
      ExternalData::DeserializerSync(isolate, context, buff);
  if (!result->IsNativeError()) {
    info.GetReturnValue().Set(result);
  } else {
    isolate->ThrowException(result);
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
  std::string script = *v8::String::Utf8Value(isolate, info[0]);
  std::string filename{""};
  if (info.Length() > 1) {
    filename = *v8::String::Utf8Value(isolate, info[1]);
  }
  context_handle->EvalAsync(std::move(async_info), std::move(script),
                            std::move(filename));

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
      {"eval", 1, EvalOperationCallback, v8::PropertyAttribute::DontDelete,
       Dependence::kPrototype},
      {"evalAsync", 1, EvalAsyncOperationCallback,
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
