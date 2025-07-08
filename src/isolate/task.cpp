#include "task.h"

#include "../utils/utils.h"
#include "external_data.h"
#include "scheduler.h"

namespace svm {

ScriptTask::ScriptTask(CallInfo& source, ResultInfo& target)
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

ScriptTaskAsync::ScriptTaskAsync(CallInfo& call_info, ResultInfo& result_info)
    : AsyncTask(result_info.scheduler),
      call_info_{std::move(call_info)},
      result_info_{std::move(result_info)} {}
ScriptTaskAsync::~ScriptTaskAsync() = default;
void ScriptTaskAsync::Run() {
  v8::Isolate* isolate = call_info_.isolate;
  v8::Locker locker(isolate);

  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope call_scope(isolate);
  v8::Local<v8::Context> context = call_info_.context.Get(isolate);
  v8::Context::Scope context_scope{context};

  v8::TryCatch try_catch(call_info_.isolate);
  v8::Local<v8::Script> script;
  v8::Local<v8::String> code = toString(call_info_.isolate, call_info_.script);
  v8::ScriptOrigin scriptOrigin =
      v8::ScriptOrigin(toString(call_info_.isolate, ""));
  if (v8::Script::Compile(context, code, &scriptOrigin).ToLocal(&script)) {
    v8::MaybeLocal<v8::Value> maybe_result = script->Run(context);
    if (!maybe_result.IsEmpty()) {
      ExternalData::SourceData source_data = {isolate, context,
                                              maybe_result.ToLocalChecked()};
      std::pair<uint8_t*, size_t> buff = ExternalData::CopyAsync(source_data);
      result_info_.scheduler->TaskRunner()->PostTask(
          std::make_unique<DeserializeTaskAsync>(result_info_, buff));
      return;
    }
  }

  if (try_catch.HasCaught()) {
  }
  try_catch.ReThrow();
}

DeserializeTaskAsync::DeserializeTaskAsync(
    ScriptTaskAsync::ResultInfo& result_info,
    std::pair<uint8_t*, size_t>& buff)
    : AsyncTask(result_info.scheduler),
      result_info_{std::move(result_info)},
      buff_(buff) {}
DeserializeTaskAsync::~DeserializeTaskAsync() = default;
void DeserializeTaskAsync::Run() {
  v8::Isolate* isolate = result_info_.isolate;
  v8::HandleScope target_scope(isolate);
  v8::Local<v8::Context> context = result_info_.context.Get(isolate);
  v8::Context::Scope context_scope{context};

  v8::ValueDeserializer deserializer(isolate, buff_.first, buff_.second);
  v8::Local<v8::Value> result;
  if (deserializer.ReadValue(context).ToLocal(&result)) {
    result_info_.resolver.Get(isolate)->Resolve(context, result);
  } else {
    result_info_.resolver.Get(isolate)->Reject(
        context, v8::Exception::Error(
                     toString(result_info_.isolate, "evalAsync failed.")));
  }
}

}  // namespace svm
