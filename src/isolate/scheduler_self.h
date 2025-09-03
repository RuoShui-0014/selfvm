#pragma once

#include "isolate/scheduler.h"

namespace svm {

template <>
class UVScheduler<Scheduler::Type::kSelf> final : public Scheduler {
public:
  explicit UVScheduler(v8::Isolate* isolate,
                       std::unique_ptr<node::ArrayBufferAllocator> allocator);
  ~UVScheduler() override;

  uv_loop_t* GetLoop() override { return uv_loop_.get(); }
  TimerManager* GetTimerManager() override { return timer_manager_.get(); }

  void PostTask(std::unique_ptr<v8::Task> task, TaskType type) override;
  uint32_t PostDelayedTask(std::unique_ptr<v8::Task> task,
                           uint64_t ms,
                           Timer::Type type) override;

  void FlushMicroTasks();
  void FlushInterruptTasks();
  void RunForegroundTask(std::unique_ptr<v8::Task> task) const;
  void FlushForegroundTasks();

  void AgentConnect(int port) const;
  void AgentDisconnect() const;
  void AgentAddContext(v8::Local<v8::Context> context,
                       const std::string& name) const;
  void AgentDispatchProtocolMessage(std::string message) const;
  void AgentDispose() const;

  void StartLoop();

  void Ref() override;
  void Unref() override;

protected:
  std::mutex mutex_macro_, mutex_micro_, mutex_interrupt_;
  TaskQueue tasks_macro_, tasks_micro_, tasks_interrupt_;


  v8::Isolate* isolate_{nullptr};
  std::unique_ptr<uv_loop_t> uv_loop_;
  std::unique_ptr<uv_async_t> uv_task_;
  std::atomic_bool working_{false};
  std::atomic<int> uv_ref_count_{1};
  std::unique_ptr<TimerManager> timer_manager_;

  node::IsolateData* isolate_data_{nullptr};
  std::unique_ptr<node::ArrayBufferAllocator> allocator_;
  std::unique_ptr<InspectorAgent> inspector_agent_;
  std::thread thread_;
};

}  // namespace svm
