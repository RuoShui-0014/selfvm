//
// Created by ruoshui on 25-7-5.
//

#pragma once

#include <v8.h>

#include "../utils/utils.h"
#include "scheduler.h"

namespace svm {

class Scheduler;

class ScriptTask : public v8::Task {
 public:
  struct CallInfo {
    v8::Isolate* isolate;
    v8::Local<v8::Context> context;
    std::string script;
  };
  struct ResultInfo {
    v8::Isolate* isolate;
    v8::Local<v8::Context> context;
    v8::Local<v8::Value>* result;
  };
  ScriptTask(CallInfo& source, ResultInfo& target);
  ~ScriptTask() override;
  void Run() override;

 private:
  CallInfo& source_;
  ResultInfo& target_;
};

class AsyncTask : public v8::Task {
 public:
  AsyncTask(Scheduler* scheduler) : scheduler_(scheduler) {
    scheduler_->KeepAlive();
  }
  ~AsyncTask() override { scheduler_->WillDie(); }

 private:
  Scheduler* scheduler_;
};

class ScriptTaskAsync : public AsyncTask {
 public:
  struct CallInfo {
    v8::Isolate* isolate;
    RemoteHandle<v8::Context> context;
    std::string script;
    CallInfo(v8::Isolate* isolate,
             v8::Local<v8::Context> context,
             const std::string& script)
        : isolate(isolate),
          context(isolate, context),
          script(std::move(script)) {}
    CallInfo(CallInfo&& call_info) noexcept
        : isolate(call_info.isolate),
          context(std::move(call_info.context)),
          script(std::move(call_info.script)) {}
  };
  struct ResultInfo {
    Scheduler* scheduler;
    v8::Isolate* isolate;
    RemoteHandle<v8::Context> context;
    RemoteHandle<v8::Promise::Resolver> resolver;
    ResultInfo(Scheduler* scheduler,
               v8::Isolate* isolate,
               v8::Local<v8::Context> context,
               v8::Local<v8::Promise::Resolver> resolver)
        : scheduler(scheduler),
          isolate(isolate),
          context(isolate, context),
          resolver(isolate, resolver) {}
    ResultInfo(ResultInfo&& result_info) noexcept
        : scheduler(result_info.scheduler),
          isolate(result_info.isolate),
          context(std::move(result_info.context)),
          resolver(std::move(result_info.resolver)) {}
  };
  ScriptTaskAsync(CallInfo& call_info, ResultInfo& result_info);
  ~ScriptTaskAsync() override;

  void Run() override;

 private:
  CallInfo call_info_;
  ResultInfo result_info_;
};

class GcTaskAsync : public v8::Task {
 public:
  explicit GcTaskAsync(v8::Isolate* isolate) : isolate_(isolate) {}
  ~GcTaskAsync() override = default;

  void Run() override { isolate_->LowMemoryNotification(); }

 private:
  v8::Isolate* isolate_;
};

class DeserializeTaskAsync : public AsyncTask {
public:
  DeserializeTaskAsync(ScriptTaskAsync::ResultInfo& result_info,
                       std::pair<uint8_t*, size_t>& buff);
  ~DeserializeTaskAsync() override;

  void Run() override;

private:
  ScriptTaskAsync::ResultInfo result_info_;
  std::pair<uint8_t*, size_t> buff_;
};

}  // namespace svm
