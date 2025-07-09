#include "task.h"

#include "../module/context_handle.h"
#include "../module/isolate_handle.h"
#include "../utils/utils.h"
#include "external_data.h"
#include "scheduler.h"

namespace svm {

ScriptTask::ScriptTask(ScriptInfo& source, ResultInfo& target)
    : source_(source), target_(target) {}
ScriptTask::~ScriptTask() = default;
void ScriptTask::Run() {
  v8::Locker locker(source_.isolate);

  v8::Isolate::Scope isolate_scope(source_.isolate);
  v8::HandleScope source_scope(source_.isolate);
  v8::Context::Scope context_scope{source_.context};

  v8::TryCatch try_catch(source_.isolate);
  v8::Local<v8::Script> script;
  v8::Local<v8::String> code = toString(source_.isolate, source_.script);
  v8::ScriptOrigin scriptOrigin =
      v8::ScriptOrigin(toString(source_.isolate, ""));
  if (v8::Script::Compile(source_.context, code, &scriptOrigin)
          .ToLocal(&script)) {
    v8::MaybeLocal<v8::Value> maybe_result = script->Run(source_.context);
    if (maybe_result.IsEmpty()) {
      return;
    }
    ExternalData::SourceData source_data = {source_.isolate, source_.context,
                                            maybe_result.ToLocalChecked()};
    {
      v8::Locker result_locker(target_.isolate);
      v8::HandleScope result_scope(target_.isolate);
      ExternalData::TargetData target_data{target_.isolate, target_.context,
                                           target_.result};
      ExternalData::CopySync(source_data, target_data);
    }
  }
}

ScriptTaskAsync::ScriptTaskAsync(AsyncInfo& async_info, ScriptInfo& script_info)
    : AsyncTask(async_info), script_info_{std::move(script_info)} {}
ScriptTaskAsync::~ScriptTaskAsync() = default;
void ScriptTaskAsync::Run() {
  v8::Isolate* isolate = script_info_.isolate;
  v8::Locker locker(isolate);

  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope call_scope(isolate);
  v8::Local<v8::Context> context = script_info_.context.Get(isolate);
  v8::Context::Scope context_scope{context};

  v8::TryCatch try_catch(isolate);
  v8::Local<v8::Script> script;
  v8::Local<v8::String> code = toString(isolate, script_info_.script);
  v8::ScriptOrigin scriptOrigin = v8::ScriptOrigin(toString(isolate, ""));
  if (v8::Script::Compile(context, code, &scriptOrigin).ToLocal(&script)) {
    v8::MaybeLocal<v8::Value> maybe_result = script->Run(context);
    if (!maybe_result.IsEmpty()) {
      ExternalData::SourceData source_data = {isolate, context,
                                              maybe_result.ToLocalChecked()};
      std::pair<uint8_t*, size_t> buff = ExternalData::CopyAsync(source_data);
      GetAsyncInfo().scheduler->TaskRunner()->PostTask(
          std::make_unique<DeserializeTaskAsync>(GetAsyncInfo(), buff));
      return;
    }
  }

  if (try_catch.HasCaught()) {
    ExternalData::SourceData source_data = {isolate, context,
                                            try_catch.Exception()};
    std::pair<uint8_t*, size_t> buff = ExternalData::CopyAsync(source_data);
    GetAsyncInfo().scheduler->TaskRunner()->PostTask(
        std::make_unique<DeserializeTaskAsync>(GetAsyncInfo(), buff));
  }
  try_catch.ReThrow();
}

CreateContextTaskAsync::CallbackTask::CallbackTask(
    AsyncInfo& async_info,
    IsolateHandle* isolate_handle,
    v8::Local<v8::Context> context)
    : AsyncTask(async_info),
      isolate_handle_(isolate_handle),
      context_{context} {}
CreateContextTaskAsync::CallbackTask::~CallbackTask() = default;
void CreateContextTaskAsync::CallbackTask::Run() {
  AsyncInfo& async_info = GetAsyncInfo();
  v8::Isolate* isolate = async_info.isolate;
  v8::HandleScope target_scope(isolate);
  v8::Local<v8::Context> context = async_info.context.Get(isolate);
  v8::Context::Scope context_scope{context};

  v8::Local<v8::FunctionTemplate> isolate_handle_template =
      V8ContextHandle::GetWrapperTypeInfo()
          ->GetV8ClassTemplate(isolate)
          .As<v8::FunctionTemplate>();
  v8::Local<v8::Object> target = isolate_handle_template->InstanceTemplate()
                                     ->NewInstance(isolate->GetCurrentContext())
                                     .ToLocalChecked();
  {
    v8::Locker locker(context_.GetIsolate());
    ScriptWrappable::Wrap(target,
                          MakeCppGcObject<GC::kSpecified, ContextHandle>(
                              isolate, isolate_handle_.Get(), context_.Get()));
  }
  async_info.resolver.Get(isolate)->Resolve(context, target);
}

CreateContextTaskAsync::CreateContextTaskAsync(AsyncInfo& async_info,
                                               IsolateHandle* isolate_handle)
    : AsyncTask(async_info), isolate_handle_(isolate_handle) {}
CreateContextTaskAsync::~CreateContextTaskAsync() = default;
void CreateContextTaskAsync::Run() {
  v8::Local<v8::Context> context =
      isolate_handle_->GetIsolateHolder()->NewContext();
  auto task = std::make_unique<CallbackTask>(GetAsyncInfo(),
                                             isolate_handle_.Get(), context);
  GetAsyncInfo().scheduler->TaskRunner()->PostTask(std::move(task));
}

ScriptTaskAsync::DeserializeTaskAsync::DeserializeTaskAsync(
    AsyncInfo& async_info,
    std::pair<uint8_t*, size_t>& buff)
    : AsyncTask(async_info), buff_(std::move(buff)) {}
ScriptTaskAsync::DeserializeTaskAsync::~DeserializeTaskAsync() = default;
void ScriptTaskAsync::DeserializeTaskAsync::Run() {
  AsyncInfo& async_info = GetAsyncInfo();
  v8::Isolate* isolate = async_info.isolate;
  v8::HandleScope target_scope(isolate);
  v8::Local<v8::Context> context = async_info.context.Get(isolate);
  v8::Context::Scope context_scope{context};

  v8::ValueDeserializer deserializer(isolate, buff_.first, buff_.second);
  v8::Local<v8::Value> result;
  if (deserializer.ReadValue(context).ToLocal(&result)) {
    async_info.resolver.Get(isolate)->Resolve(context, result);
  } else {
    async_info.resolver.Get(isolate)->Reject(
        context, v8::Exception::Error(toString(isolate, "evalAsync failed.")));
  }
}

}  // namespace svm
