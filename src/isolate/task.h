//
// Created by ruoshui on 25-7-5.
//

#pragma once

#include <v8.h>

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
  class Waiter {
   public:
    Waiter() = default;
    ~Waiter() = default;

    T& GetResult() {
      while (true) {
        if (flag.load()) {
          return result_;
        }
      }

      // std::unique_lock<std::mutex> lock(mutex_);
      // condition_.wait(lock, [this] { return flag.load(); });
      // return result_;
    }
    void SetResult(const T& result) {
      result_ = result;
      flag.store(true);

      // {
      //   std::lock_guard<std::mutex> lock(mutex_);
      //   result_ = result;
      //   flag.store(true);
      // }
      // condition_.notify_one();
    }

   private:
    friend class SyncTask;
    // std::mutex mutex_;
    // std::condition_variable condition_;
    std::atomic<bool> flag{false};
    T result_;
  };

  explicit SyncTask() = default;
  ~SyncTask() override { wait_->flag.store(true); }

  std::unique_ptr<Waiter> CreateWaiter() {
    assert(!wait_);

    auto wait = std::make_unique<Waiter>();
    wait_ = wait.get();
    return wait;
  }

 protected:
  Waiter* wait_{nullptr};
};

template <typename T>
  requires std::is_void_v<T>
class SyncTask<T> : public v8::Task {
 public:
  explicit SyncTask() = default;
  ~SyncTask() override = default;
};

class CreateContextTask : public SyncTask<uint32_t> {
 public:
  explicit CreateContextTask(IsolateHandle* isolate_handle);
  ~CreateContextTask() override = default;

  void Run() override;

 private:
  cppgc::Member<IsolateHandle> isolate_handle_;
};

class ScriptTask final : public SyncTask<std::pair<uint8_t*, size_t>> {
 public:
  ScriptTask(ContextHandle* context_handle, std::string script, std::string filename);
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
            RemoteHandle<v8::Promise::Resolver> resolver)
      : isolate_handle(isolate_handle),
        isolate(isolate),
        context(context),
        resolver(resolver) {
    isolate_handle->GetIsolateHolder()->GetSchedulerPar()->KeepAlive();
  }
  ~AsyncInfo() {
    isolate_handle->GetIsolateHolder()->GetSchedulerPar()->WillDie();
  }
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
                           std::string script, std::string filename);
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
    explicit Callback(std::unique_ptr<AsyncInfo> info, uint32_t id);
    ~Callback() override = default;

    void Run() override;

    std::unique_ptr<AsyncInfo> info_;
    uint32_t id_;
  };
  explicit CreateContextAsyncTask(std::unique_ptr<AsyncInfo> info);
  ~CreateContextAsyncTask() override = default;

  void Run() override;
};

}  // namespace svm
