//
// Created by ruoshui on 25-7-3.
//

#pragma once

#include <node.h>
#include <uv.h>
#include <v8-platform.h>

#include <iostream>
#include <mutex>
#include <queue>

#include "platform_delegate.h"

namespace svm {

class Scheduler {
 public:
  Scheduler() = default;
  virtual ~Scheduler() = default;

  static void RegisterIsolate(v8::Isolate* isolate, uv_loop_t* loop);
  static uv_loop_t* GetIsolateUvLoop(v8::Isolate* isolate);

  virtual void RunLoop() {}
  virtual uv_loop_t* GetUVLoop() const { return nullptr; }
  virtual std::shared_ptr<v8::TaskRunner> TaskRunner() const { return {}; }
  virtual void KeepAlive() {}
  virtual void WillDie() {}
};

class IsolateScheduler : public Scheduler,
                         public node::IsolatePlatformDelegate,
                         public v8::TaskRunner,
                         public std::enable_shared_from_this<IsolateScheduler> {
 public:
  IsolateScheduler();
  ~IsolateScheduler() override;

  // node::IsolatePlatformDelegate overrides
  std::shared_ptr<v8::TaskRunner> GetForegroundTaskRunner() override {
    return shared_from_this();
  }
  bool IdleTasksEnabled() override { return false; }

  // v8::TaskRunner override
  bool NonNestableTasksEnabled() const override { return true; }
  bool NonNestableDelayedTasksEnabled() const override { return true; }
  void PostTask(std::unique_ptr<v8::Task> task) override;
  void PostDelayedTask(std::unique_ptr<v8::Task> task,
                       double delay_in_seconds) override;
  void PostNonNestableTask(std::unique_ptr<v8::Task> task) override;
};

template <bool Is_Self>
class UVScheduler : public Scheduler {
 public:
  explicit UVScheduler(v8::Isolate* isolate);
  ~UVScheduler() override;

  void RunLoop();
  void KeepAlive() override;
  void WillDie() override;
  std::shared_ptr<v8::TaskRunner> TaskRunner() const override;
  uv_loop_t* GetUVLoop() const override { return uv_loop_; }
  void Stop() { uv_async_send(keep_alive_); }

 private:
  v8::Isolate* isolate_;
  node::IsolateData* isolate_data_;
  node::ArrayBufferAllocator* allocator_;
  uv_loop_t* uv_loop_;
  uv_async_t* keep_alive_;
  std::atomic<int> uv_ref_count{0};

  using ThreadType = std::conditional_t<Is_Self, std::thread, std::nullptr_t>;
  ThreadType thread_;
};

template <bool Is_Self>
UVScheduler<Is_Self>::UVScheduler(v8::Isolate* isolate) : isolate_(isolate) {
  if constexpr (Is_Self) {
    uv_loop_ = new uv_loop_t;
    uv_loop_init(uv_loop_);

    Scheduler::RegisterIsolate(isolate_, uv_loop_);

    keep_alive_ = new uv_async_t;
    uv_async_init(uv_loop_, keep_alive_,
                  [](uv_async_t* handle) { uv_stop(handle->loop); });
  } else {
    uv_loop_ = Scheduler::GetIsolateUvLoop(isolate_);
    keep_alive_ = new uv_async_t;
    uv_async_init(uv_loop_, keep_alive_, [](uv_async_t*) {});
    uv_unref(reinterpret_cast<uv_handle_t*>(keep_alive_));
  }
}

template <bool Is_Self>
UVScheduler<Is_Self>::~UVScheduler() {
  if constexpr (Is_Self) {
    uv_async_send(keep_alive_);
    if (thread_.joinable()) {
      thread_.join();
    }
    delete uv_loop_;
  } else {
    uv_close(reinterpret_cast<uv_handle_t*>(keep_alive_),
             [](uv_handle_t* handle) {
               delete reinterpret_cast<uv_async_t*>(handle);
             });
  }
}

template <bool Is_Self>
void UVScheduler<Is_Self>::RunLoop() {
  if constexpr (Is_Self) {
    thread_ = std::thread([this]() {
      {
        v8::Locker locker(isolate_);
        v8::Isolate::Scope isolate_scope(isolate_);
        v8::HandleScope handle_scope(isolate_);

        isolate_data_ = node::CreateIsolateData(
            isolate_, uv_loop_, PlatformDelegate::GetNodePlatform());

        uv_run(uv_loop_, UV_RUN_DEFAULT);
      }

      uv_close(reinterpret_cast<uv_handle_t*>(keep_alive_),
               [](uv_handle_t* handle) {
                 delete reinterpret_cast<uv_async_t*>(handle);
               });
      uv_loop_close(uv_loop_);

      node::FreeIsolateData(isolate_data_);
      PlatformDelegate::UnregisterIsolate(isolate_);
      isolate_->Dispose();
    });
  }
}

template <bool Is_Self>
void UVScheduler<Is_Self>::KeepAlive() {
  if (++uv_ref_count == 1) {
    uv_ref(reinterpret_cast<uv_handle_t*>(keep_alive_));
  }
}

template <bool Is_Self>
void UVScheduler<Is_Self>::WillDie() {
  if (--uv_ref_count == 0) {
    uv_unref(reinterpret_cast<uv_handle_t*>(keep_alive_));
  }
}

template <bool Is_Self>
std::shared_ptr<v8::TaskRunner> UVScheduler<Is_Self>::TaskRunner() const {
  if (isolate_) {
    return PlatformDelegate::GetNodePlatform()->GetForegroundTaskRunner(
        isolate_);
  }
  return {};
}
}  // namespace svm
