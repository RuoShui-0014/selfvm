//
// Created by ruoshui on 25-7-5.
//

#pragma once

#include <v8.h>

#include <future>

#include "../module/context_handle.h"
#include "../module/isolate_handle.h"
#include "../utils/utils.h"

namespace svm {

class ContextHandle;
class IsolateHandle;

// 同步任务
template <typename T>
class SyncTask : public v8::Task {
 public:
  explicit SyncTask() = default;
  ~SyncTask() override = default;

  std::future<T> GetFuture() { return promise_.get_future(); }
  void SetResult(const T& _Val) { promise_.set_value(_Val); }

 protected:
  std::promise<T> promise_;
};

template <typename T>
  requires std::is_void_v<T>
class SyncTask<T> : public v8::Task {
 public:
  explicit SyncTask() = default;
  ~SyncTask() override = default;
};

class CreateContextTask : public SyncTask<v8::Context*> {
 public:
  explicit CreateContextTask(IsolateHandle* isolate_handle);
  ~CreateContextTask() override = default;

  void Run() override;

 private:
  cppgc::Member<IsolateHandle> isolate_handle_;
};

class ScriptTask final : public SyncTask<std::pair<uint8_t*, size_t>> {
 public:
  ScriptTask(ContextHandle* context_handle,
             std::string script,
             std::string filename);
  ~ScriptTask() override = default;

  void Run() override;

 private:
  cppgc::Member<ContextHandle> context_handle_;
  std::string script_;
  std::string filename_;
};

class IsolateGcTask final : public SyncTask<void> {
 public:
  explicit IsolateGcTask(v8::Isolate* isolate);
  ~IsolateGcTask() override = default;

  void Run() override;

 private:
  v8::Isolate* isolate_;
};

class AsyncInfo {
 public:
  IsolateHandle* isolate_handle;
  v8::Isolate* isolate;
  RemoteHandle<v8::Context> context;
  RemoteHandle<v8::Promise::Resolver> resolver;

  AsyncInfo(IsolateHandle* isolate_handle,
            v8::Isolate* isolate,
            RemoteHandle<v8::Context> context,
            RemoteHandle<v8::Promise::Resolver> resolver);
  ~AsyncInfo();
};
class AsyncTask : public v8::Task {
 public:
  explicit AsyncTask(std::unique_ptr<AsyncInfo> info)
      : info_(std::move(info)) {}
  ~AsyncTask() override = default;

 protected:
  std::unique_ptr<AsyncInfo> info_;
};

class ScriptAsyncTask final : public AsyncTask {
 public:
  class Callback : public v8::Task {
   public:
    explicit Callback(std::unique_ptr<AsyncInfo> info,
                      std::pair<uint8_t*, size_t> buff);
    ~Callback() override = default;

    void Run() override;

   private:
    std::unique_ptr<AsyncInfo> info_;
    std::pair<uint8_t*, size_t> buff_;
  };
  explicit ScriptAsyncTask(std::unique_ptr<AsyncInfo> info,
                           ContextHandle* context_handle,
                           std::string script,
                           std::string filename);
  ~ScriptAsyncTask() override = default;

  void Run() override;

 private:
  cppgc::Member<ContextHandle> context_handle_;
  std::string script_;
  std::string filename_;
};

class CreateContextAsyncTask final : public AsyncTask {
 public:
  class Callback : public v8::Task {
   public:
    explicit Callback(std::unique_ptr<AsyncInfo> info, v8::Context* address);
    ~Callback() override = default;

    void Run() override;

    std::unique_ptr<AsyncInfo> info_;
    v8::Context* const address_;
  };
  explicit CreateContextAsyncTask(std::unique_ptr<AsyncInfo> info);
  ~CreateContextAsyncTask() override = default;

  void Run() override;
};

}  // namespace svm
