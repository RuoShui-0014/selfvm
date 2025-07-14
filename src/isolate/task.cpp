#include "task.h"

#include "../isolate/isolate_holder.h"
#include "../module/context_handle.h"
#include "../module/isolate_handle.h"
#include "../utils/utils.h"
#include "external_data.h"

namespace svm {

CreateContextTask::CreateContextTask(IsolateHandle* isolate_handle)
    : isolate_handle_(isolate_handle) {}
void CreateContextTask::Run() {
  uint32_t id = isolate_handle_->GetIsolateHolder()->NewContext();
  this->wait_->SetResult(id);
}

ScriptTask::ScriptTask(ContextHandle* context_handle,
                       std::string script,
                       std::string filename)
    : context_handle_(context_handle),
      script_(std::move(script)),
      filename_(std::move(filename)) {}
void ScriptTask::Run() {
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
      this->wait_->SetResult(ExternalData::CopySync(data));
      return;
    }
  }

  if (try_catch.HasCaught()) {
    ExternalData::SourceData data{isolate, context, try_catch.Exception()};
    this->wait_->SetResult(ExternalData::CopySync(data));
    try_catch.Reset();
  }
}

IsolateGcTask::IsolateGcTask(v8::Isolate* isolate) : isolate_{isolate} {}
void IsolateGcTask::Run() {
  isolate_->LowMemoryNotification();
}

AsyncInfo::AsyncInfo(IsolateHandle* isolate_handle,
                     v8::Isolate* isolate,
                     RemoteHandle<v8::Context> context,
                     RemoteHandle<v8::Promise::Resolver> resolver)
    : isolate_handle(isolate_handle),
      isolate(isolate),
      context(context),
      resolver(resolver) {
  isolate_handle->GetSchedulerPar()->KeepAlive();
}
AsyncInfo::~AsyncInfo() {
  isolate_handle->GetSchedulerPar()->WillDie();
}

ScriptAsyncTask::Callback::Callback(std::unique_ptr<AsyncInfo> info,
                                    std::pair<uint8_t*, size_t> buff)
    : info_(std::move(info)), buff_(std::move(buff)) {}
void ScriptAsyncTask::Callback::Run() {
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

ScriptAsyncTask::ScriptAsyncTask(std::unique_ptr<AsyncInfo> info,
                                 ContextHandle* context_handle,
                                 std::string script,
                                 std::string filename)
    : AsyncTask(std::move(info)),
      context_handle_(context_handle),
      script_(std::move(script)),
      filename_(std::move(filename)) {}
void ScriptAsyncTask::Run() {
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
      auto buff = ExternalData::CopyAsync(data);
      context_handle_->PostTaskToPar(
          std::make_unique<ScriptAsyncTask::Callback>(std::move(info_), buff));
      return;
    }
  }

  if (try_catch.HasCaught()) {
    ExternalData::SourceData data{isolate, context, try_catch.Exception()};
    auto buff = ExternalData::CopyAsync(data);
    context_handle_->PostTaskToPar(
        std::make_unique<ScriptAsyncTask::Callback>(std::move(info_), buff));
    try_catch.Reset();
  }
}
CreateContextAsyncTask::Callback::Callback(std::unique_ptr<AsyncInfo> info,
                                           uint32_t id)
    : info_(std::move(info)), id_(id) {}
void CreateContextAsyncTask::Callback::Run() {
  v8::Isolate* isolate = info_->isolate;
  v8::Local<v8::Context> context = info_->context.Get(isolate);
  v8::HandleScope source_scope(isolate);

  ContextHandle* context_handle =
      MakeCppGcObject<GC::kSpecified, ContextHandle>(
          isolate, info_->isolate_handle, id_);
  v8::Local<v8::FunctionTemplate> isolate_handle_template =
      V8ContextHandle::GetWrapperTypeInfo()
          ->GetV8ClassTemplate(isolate)
          .As<v8::FunctionTemplate>();
  v8::Local<v8::Object> obj = isolate_handle_template->InstanceTemplate()
                                  ->NewInstance(isolate->GetCurrentContext())
                                  .ToLocalChecked();
  ScriptWrappable::Wrap(obj, context_handle);
  info_->resolver.Get(isolate)->Resolve(context, obj);
}
CreateContextAsyncTask::CreateContextAsyncTask(std::unique_ptr<AsyncInfo> info)
    : AsyncTask(std::move(info)) {}
void CreateContextAsyncTask::Run() {
  uint32_t id = info_->isolate_handle->GetIsolateHolder()->NewContext();
  info_->isolate_handle->PostTaskToPar(
      std::make_unique<Callback>(std::move(info_), id));
}

}  // namespace svm
