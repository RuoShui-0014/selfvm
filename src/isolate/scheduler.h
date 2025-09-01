#pragma once

#include <node.h>
#include <uv.h>
#include <v8-platform.h>

#include <mutex>
#include <queue>

#include "native/timer_manager.h"

namespace svm {

class TimerManager;
class InspectorAgent;

class Scheduler {
 public:
  enum class TaskType { kMacro, kMicro, kInterrupt };
  using TaskQueue = std::queue<std::unique_ptr<v8::Task>>;

  virtual ~Scheduler() = default;

  static void RegisterIsolateScheduler(v8::Isolate* isolate,
                                       Scheduler* scheduler);
  static void UnregisterIsolateScheduler(v8::Isolate* isolate);
  static Scheduler* GetIsolateScheduler(v8::Isolate* isolate);

  virtual uv_loop_t* GetLoop() { return nullptr; }
  virtual std::shared_ptr<v8::TaskRunner> TaskRunner() { return {}; }
  virtual TimerManager* GetTimerManager() { return nullptr; }

  virtual void PostTask(std::unique_ptr<v8::Task> task, TaskType type) = 0;
  virtual uint32_t PostDelayedTask(std::unique_ptr<v8::Task> task,
                                   uint64_t ms,
                                   Timer::Type type) = 0;

  virtual void Ref() {}
  virtual void Unref() {}
};

class UVScheduler : public Scheduler {
 public:
  explicit UVScheduler(v8::Isolate* isolate);
  ~UVScheduler() override;

  uv_loop_t* GetLoop() override { return uv_loop_; }
  std::shared_ptr<v8::TaskRunner> TaskRunner() override;
  TimerManager* GetTimerManager() override { return timer_manager_.get(); }

  void PostTask(std::unique_ptr<v8::Task> task, TaskType type) override;
  uint32_t PostDelayedTask(std::unique_ptr<v8::Task> task,
                           uint64_t ms,
                           Timer::Type type) override;

  void Ref() override;
  void Unref() override;

 protected:
  v8::Isolate* isolate_{nullptr};
  uv_loop_t* uv_loop_{nullptr};
  uv_async_t* uv_task_{nullptr};
  std::atomic<int> uv_ref_count_{1};
  std::shared_ptr<v8::TaskRunner> task_runner_;
  std::unique_ptr<TimerManager> timer_manager_;
};

class UVSchedulerSel final : public UVScheduler {
 public:
  explicit UVSchedulerSel(
      v8::Isolate* isolate,
      std::unique_ptr<node::ArrayBufferAllocator> allocator);
  ~UVSchedulerSel() override;

  void PostTask(std::unique_ptr<v8::Task> task, TaskType type) override;
  uint32_t PostDelayedTask(std::unique_ptr<v8::Task> task,
                           uint64_t ms,
                           Timer::Type type) override;

  void FlushMicroTasks();
  void FlushInterruptTasks();
  void RunForegroundTask(std::unique_ptr<v8::Task> task) const;
  void FlushForegroundTasksInternal();

  void AgentConnect(int port) const;
  void AgentDisconnect() const;
  void AgentAddContext(v8::Local<v8::Context> context,
                       const std::string& name) const;
  void AgentDispatchProtocolMessage(std::string message) const;
  void AgentDispose() const;

  void StartLoop();

 private:
  std::mutex mutex_macro_, mutex_micro_, mutex_interrupt_;
  TaskQueue tasks_macro_, tasks_micro_, tasks_interrupt_;
  std::atomic_bool running_{false};

  node::IsolateData* isolate_data_{nullptr};
  std::unique_ptr<node::ArrayBufferAllocator> allocator_;
  std::unique_ptr<InspectorAgent> inspector_agent_;
  std::thread thread_;
};

class UVSchedulerPar final : public UVScheduler {
 public:
  explicit UVSchedulerPar(v8::Isolate* isolate, uv_loop_t* uv_loop);
  ~UVSchedulerPar() override;

  void PostTask(std::unique_ptr<v8::Task> task, TaskType type) override;

  void FlushMicroTasks();
  void FlushInterruptTasks();
  void RunForegroundTask(std::unique_ptr<v8::Task> task) const;
  void FlushForegroundTasksInternal();

  static UVSchedulerPar* nodejs_scheduler;

 private:
  std::mutex mutex_macro_, mutex_micro_, mutex_interrupt_;
  TaskQueue tasks_macro_, tasks_micro_, tasks_interrupt_;
  std::atomic_bool running{false};
};

}  // namespace svm
