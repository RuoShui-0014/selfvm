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

class TimerManager;
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
  TimerManager* GetTimerManager() const { return timer_manager_.get(); }

  virtual void PostMacroTask(std::unique_ptr<v8::Task> task);
  virtual void PostDelayedTask(std::unique_ptr<v8::Task> task, uint64_t ms);
  virtual void PostMicroTask(std::unique_ptr<v8::Task> task);
  virtual void PostInterruptTask(std::unique_ptr<v8::Task> task);

  virtual void FlushForegroundTasksInternal();

  void Ref();
  void Unref();

 protected:
  v8::Isolate* isolate_{nullptr};
  uv_loop_t* uv_loop_{nullptr};
  uv_async_t* uv_task_{nullptr};
  std::atomic<int> uv_ref_count{0};
  std::shared_ptr<v8::TaskRunner> task_runner_;
  std::unique_ptr<TimerManager> timer_manager_;
};

class UVSchedulerSel final : public Scheduler {
 public:
  explicit UVSchedulerSel(
      v8::Isolate* isolate,
      std::unique_ptr<node::ArrayBufferAllocator> allocator);
  ~UVSchedulerSel() override;

  void PostMacroTask(std::unique_ptr<v8::Task> task) override;
  void PostMicroTask(std::unique_ptr<v8::Task> task) override;
  void PostInterruptTask(std::unique_ptr<v8::Task> task) override;

  void RunForegroundTask(std::unique_ptr<v8::Task> task) const;
  void FlushForegroundTasksInternal() override;

  void AgentConnect(int port) const;
  void AgentDisconnect() const;
  void AgentAddContext(v8::Local<v8::Context> context,
                       const std::string& name) const;
  void AgentDispatchProtocolMessage(std::string message) const;
  void AgentDispose() const;

  void StartLoop();
  void RunInterruptTasks();

 private:
  std::mutex mutex_task_;
  TaskQueue tasks_macro_, tasks_micro_, tasks_interrupt_;
  std::atomic_bool running_{false};

  node::IsolateData* isolate_data_{nullptr};
  std::unique_ptr<node::ArrayBufferAllocator> allocator_;
  std::unique_ptr<InspectorAgent> inspector_agent_;
  std::thread thread_;
};

class UVSchedulerPar final : public Scheduler {
 public:
  explicit UVSchedulerPar(v8::Isolate* isolate, uv_loop_t* uv_loop);
  ~UVSchedulerPar() override;

  void PostMacroTask(std::unique_ptr<v8::Task> task) override;
  void PostMicroTask(std::unique_ptr<v8::Task> task) override;
  void PostInterruptTask(std::unique_ptr<v8::Task> task) override;

  void RunForegroundTask(std::unique_ptr<v8::Task> task) const;
  void FlushForegroundTasksInternal() override;

  static UVSchedulerPar* nodejs_scheduler;

 private:
  std::mutex mutex_task_;
  TaskQueue tasks_macro_, tasks_micro_, tasks_interrupt_;
  std::atomic_bool running{false};
};

}  // namespace svm
