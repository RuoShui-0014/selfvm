//
// Created by ruoshui on 25-7-3.
//

#pragma once

#include <node.h>
#include <v8-platform.h>

#include <mutex>
#include <queue>

namespace svm {

class ScriptTask : public v8::Task {
 public:
  ScriptTask(v8::Isolate* isolate,
             v8::Local<v8::Context> context,
             std::string& script);
  ~ScriptTask() override;
  void Run() override;
  v8::Local<v8::Value> GetResult() { return result.Get(isolate_); }

 private:
  v8::Isolate* isolate_;
  v8::Global<v8::Context> context_;
  std::string script_;
  v8::Global<v8::Value> result;
};

class Scheduler : public node::IsolatePlatformDelegate,
                  public v8::TaskRunner,
                  public std::enable_shared_from_this<Scheduler> {
  using TaskQueue = std::queue<std::unique_ptr<v8::Task>>;

 public:
  Scheduler();
  ~Scheduler() override;

  // node::IsolatePlatformDelegate overrides
  std::shared_ptr<v8::TaskRunner> GetForegroundTaskRunner() override {
    return shared_from_this();
  }
  bool IdleTasksEnabled() override { return false; }

  // v8::TaskRunner override
  bool NonNestableTasksEnabled() const override { return true; }
  void PostTask(std::unique_ptr<v8::Task> task) final;
  void PostDelayedTask(std::unique_ptr<v8::Task> task,
                       double delay_in_seconds) final;
  void PostNonNestableTask(std::unique_ptr<v8::Task> task) final;

  void RunTask();
  void Entry();

  TaskQueue tasks_;
  TaskQueue handle_tasks_;
  TaskQueue interrupts_;
  TaskQueue sync_interrupts_;

  auto Lock() {
    // TODO: Would be nice to find a way to roll this into the lockable
    // abstraction
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
    return Lock{*this, queue_mutex_};
  }

 private:
  mutable std::mutex queue_mutex_;
  mutable std::mutex exec_mutex_;
  std::condition_variable cv_;

  std::thread thread_;
};

}  // namespace svm
