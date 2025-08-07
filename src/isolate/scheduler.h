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

class InspectorAgent;

class Scheduler {
 public:
  using TaskQueue = std::queue<std::unique_ptr<v8::Task>>;
  explicit Scheduler(v8::Isolate* isolate);
  virtual ~Scheduler();

  static void RegisterIsolateScheduler(v8::Isolate* isolate, Scheduler* loop);
  static void UnregisterIsolateScheduler(v8::Isolate* isolate);
  static Scheduler* GetIsolateScheduler(v8::Isolate* isolate);

  uv_loop_t* GetLoop() const { return uv_loop_; }
  std::shared_ptr<v8::TaskRunner> TaskRunner();

  void PostTask(std::unique_ptr<v8::Task> task);
  void PostDelayedTask(std::unique_ptr<v8::Task> task, double delay);
  void PostHandleTask(std::unique_ptr<v8::Task> task);
  void PostInterruptTask(std::unique_ptr<v8::Task> task);

  void RunForegroundTask(std::unique_ptr<v8::Task> task) const;
  void FlushForegroundTasksInternal();

  void KeepAlive();
  void WillDie();

 protected:
  std::mutex mutex_task_;
  TaskQueue tasks_, tasks_handle_, tasks_interrupts_;
  std::atomic<bool> running{false};

  v8::Isolate* isolate_{};
  uv_loop_t* uv_loop_{};
  uv_async_t* uv_task_{};
  std::atomic<int> uv_ref_count{0};
  std::shared_ptr<v8::TaskRunner> task_runner_;
};

class UVSchedulerSel final : public Scheduler {
 public:
  explicit UVSchedulerSel(
      v8::Isolate* isolate,
      std::unique_ptr<node::ArrayBufferAllocator> allocator);
  ~UVSchedulerSel() override;

  void AgentConnect(int port) const;
  void AgentDisconnect() const;
  void AgentAddContext(v8::Local<v8::Context> context, const std::string& name) const;
  void AgentDispatchProtocolMessage(std::string message) const;
  void AgentDispose() const;

  void StartLoop();
  void RunInterruptTasks();

 private:
  node::IsolateData* isolate_data_{nullptr};
  std::unique_ptr<node::ArrayBufferAllocator> allocator_;
  std::unique_ptr<InspectorAgent> inspector_agent_;
  std::thread thread_;
};

class UVSchedulerPar final : public Scheduler {
 public:
  explicit UVSchedulerPar(v8::Isolate* isolate, uv_loop_t* uv_loop);
  ~UVSchedulerPar() override;

  static UVSchedulerPar* nodejs_scheduler;
};

}  // namespace svm
