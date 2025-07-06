//
// Created by ruoshui on 25-7-3.
//

#pragma once

#include <node.h>
#include <uv.h>
#include <v8-platform.h>

#include <mutex>
#include <queue>

namespace svm {

class Scheduler : public node::IsolatePlatformDelegate,
                  public v8::TaskRunner,
                  public std::enable_shared_from_this<Scheduler> {
 public:
  using TaskQueue = std::queue<std::unique_ptr<v8::Task>>;

  auto Lock() {
    class Lock {
     public:
      auto operator*() -> auto& { return scheduler; }
      auto operator->() { return &scheduler; }

      explicit Lock(Scheduler& scheduler, std::mutex& mutex)
          : lock{mutex}, scheduler{scheduler} {}

     private:
      std::unique_lock<std::mutex> lock;
      Scheduler& scheduler;
    };
    return Lock{*this, mutex_};
  }

  Scheduler() = default;
  explicit Scheduler(v8::Isolate* isolate, uv_loop_t* loop);
  ~Scheduler() override;

  // node::IsolatePlatformDelegate overrides
  std::shared_ptr<v8::TaskRunner> GetForegroundTaskRunner() override {
    return shared_from_this();
  }
  bool IdleTasksEnabled() override { return false; }

  // v8::TaskRunner override
  bool NonNestableTasksEnabled() const override { return true; }
  bool NonNestableDelayedTasksEnabled() const override { return true; }
  void PostTask(std::unique_ptr<v8::Task> task) final;
  void PostDelayedTask(std::unique_ptr<v8::Task> task,
                       double delay_in_seconds) final;
  void PostNonNestableTask(std::unique_ptr<v8::Task> task) final;

  static void RunTask(uv_async_t* uv_async);
  void Send() const;

 private:
  mutable std::mutex mutex_;
  std::condition_variable cv_;
  TaskQueue tasks_;
  TaskQueue handle_tasks_;
  TaskQueue interrupts_;

  v8::Isolate* isolate_;
  uv_loop_t* uv_loop_;
  uv_async_t* flush_ = nullptr;
  uv_idle_t* keep_alive_ = nullptr;
  std::thread thread_;
};

}  // namespace svm
