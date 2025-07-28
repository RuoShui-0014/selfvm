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
  using TaskQueue = std::queue<std::unique_ptr<v8::Task>>;
  explicit Scheduler(v8::Isolate* isolate);
  virtual ~Scheduler();

  static void RegisterIsolateLoop(v8::Isolate* isolate, uv_loop_t* loop);
  static void UnregisterIsolateLoop(v8::Isolate* isolate);
  static uv_loop_t* GetIsolateLoop(v8::Isolate* isolate);

  uv_loop_t* GetLoop() const { return uv_loop_; }
  std::shared_ptr<v8::TaskRunner> TaskRunner();
  void PostTask(std::unique_ptr<v8::Task> task);
  void PostDelayedTask(std::unique_ptr<v8::Task> task, double delay);
  void PostHandleTask(std::unique_ptr<v8::Task> task);
  void PostInterruptTask(std::unique_ptr<v8::Task> task);
  void RunTasks();
  void KeepAlive();
  void WillDie();

 protected:
  std::mutex mutex_task_;
  TaskQueue tasks_;
  TaskQueue tasks_handle_;
  TaskQueue tasks_interrupts_;

  v8::Isolate* isolate_{};
  uv_loop_t* uv_loop_{};
  uv_async_t* uv_task_{};
  std::atomic<int> uv_ref_count{0};
  std::shared_ptr<v8::TaskRunner> task_runner_;
};

class UVSchedulerSel : public Scheduler {
 public:
  explicit UVSchedulerSel(
      v8::Isolate* isolate,
      std::unique_ptr<node::ArrayBufferAllocator> allocator);
  ~UVSchedulerSel() override;

  void StartLoop();
  void RunInterruptTasks();

 private:
  node::IsolateData* isolate_data_{nullptr};
  std::unique_ptr<node::ArrayBufferAllocator> allocator_;

  std::thread thread_;
  std::atomic<bool> running{false};
};

class UVSchedulerPar : public Scheduler {
 public:
  explicit UVSchedulerPar(v8::Isolate* isolate);
  ~UVSchedulerPar() override;
};

}  // namespace svm
