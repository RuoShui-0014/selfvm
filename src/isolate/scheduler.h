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

class Scheduler {
 public:
  Scheduler() = default;
  virtual ~Scheduler() = default;

  virtual uv_loop_t* GetUvLoop() const { return nullptr; }
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
  void PostTask(std::unique_ptr<v8::Task> task) final;
  void PostDelayedTask(std::unique_ptr<v8::Task> task,
                       double delay_in_seconds) final;
  void PostNonNestableTask(std::unique_ptr<v8::Task> task) final;
};

class UVScheduler : public Scheduler {
 public:
  UVScheduler();
  explicit UVScheduler(v8::Isolate* isolate, uv_loop_t* loop);
  ~UVScheduler() override;

  void KeepAlive() override;
  void WillDie() override;
  std::shared_ptr<v8::TaskRunner> TaskRunner() const override;
  uv_loop_t* GetUvLoop() const override { return uv_loop_; }

 private:
  std::mutex mutex_;
  bool is_self_;

  v8::Isolate* isolate_;
  uv_loop_t* uv_loop_;
  uv_async_t* keep_alive_;
  std::atomic<int> uv_ref_count{0};
  std::thread thread_;
};

}  // namespace svm
