//
// Created by ruoshui on 25-7-5.
//

#pragma once

#include <v8.h>

#include "../utils/utils.h"
#include "isolate_holder.h"
#include "scheduler.h"

namespace svm {

class Scheduler;

class ScriptTask : public v8::Task {
 public:
  struct ScriptInfo {
    v8::Isolate* isolate;
    v8::Local<v8::Context> context;
    std::string script;
  };
  struct ResultInfo {
    v8::Isolate* isolate;
    v8::Local<v8::Context> context;
    v8::Local<v8::Value>* result;
  };
  ScriptTask(ScriptInfo& source, ResultInfo& target);
  ~ScriptTask() override;
  void Run() override;

 private:
  ScriptInfo& source_;
  ResultInfo& target_;
};

class AsyncTask : public v8::Task {
 public:
  struct AsyncInfo {
    Scheduler* scheduler;
    v8::Isolate* isolate;
    RemoteHandle<v8::Context> context;
    RemoteHandle<v8::Promise::Resolver> resolver;

    AsyncInfo(Scheduler* scheduler,
              v8::Isolate* isolate,
              v8::Local<v8::Context> context,
              v8::Local<v8::Promise::Resolver> resolver)
        : scheduler(scheduler),
          isolate(isolate),
          context(isolate, context),
          resolver(isolate, resolver) {}
    AsyncInfo(AsyncInfo&& async_info) noexcept
        : scheduler(async_info.scheduler),
          isolate(async_info.isolate),
          context(std::move(async_info.context)),
          resolver(std::move(async_info.resolver)) {}
  };
  explicit AsyncTask(AsyncInfo& async_info)
      : async_info_{std::move(async_info)} {
    async_info_.scheduler->KeepAlive();
  }
  ~AsyncTask() override { async_info_.scheduler->WillDie(); }

  AsyncInfo& GetAsyncInfo() { return async_info_; }

 private:
  AsyncInfo async_info_;
};

class ScriptTaskAsync : public AsyncTask {
 public:
  struct ScriptInfo {
    v8::Isolate* isolate;
    RemoteHandle<v8::Context> context;
    std::string script;
    ScriptInfo(v8::Isolate* isolate,
               v8::Local<v8::Context> context,
               const std::string& script)
        : isolate(isolate),
          context(isolate, context),
          script(std::move(script)) {}
    ScriptInfo(ScriptInfo&& script_info) noexcept
        : isolate(script_info.isolate),
          context(std::move(script_info.context)),
          script(std::move(script_info.script)) {}
  };
  class DeserializeTaskAsync : public AsyncTask {
  public:
    DeserializeTaskAsync(AsyncInfo& async_info,
                         std::pair<uint8_t*, size_t>& buff);
    ~DeserializeTaskAsync() override;

    void Run() override;

  private:
    std::pair<uint8_t*, size_t> buff_;
  };
  ScriptTaskAsync(AsyncInfo& async_info, ScriptInfo& script_info);
  ~ScriptTaskAsync() override;

  void Run() override;

 private:
  ScriptInfo script_info_;
};

class ContextHandle;
class IsolateHandle;

class CreateContextTaskAsync : public AsyncTask {
 public:
  class CallbackTask : public AsyncTask {
  public:
    CallbackTask(AsyncInfo& async_info, IsolateHandle* isolate_handle, v8::Local<v8::Context> context);
    ~CallbackTask() override;

    void Run() override;

  private:
    cppgc::WeakMember<IsolateHandle> isolate_handle_;
    RemoteHandle<v8::Context> context_;
  };
  explicit CreateContextTaskAsync(AsyncInfo& async_info, IsolateHandle* isolate_handle);
  ~CreateContextTaskAsync() override;

  void Run() override;

private:
  cppgc::WeakMember<IsolateHandle> isolate_handle_;
};

class GcTaskAsync : public v8::Task {
 public:
  explicit GcTaskAsync(v8::Isolate* isolate) : isolate_(isolate) {}
  ~GcTaskAsync() override = default;

  void Run() override { isolate_->LowMemoryNotification(); }

 private:
  v8::Isolate* isolate_;
};

}  // namespace svm
