#include "scheduler_self.h"

#include "inspector_agent.h"
#include "platform_delegate.h"

namespace svm {

UVScheduler<Scheduler::Type::kSelf>::UVScheduler(
    v8::Isolate* isolate,
    std::unique_ptr<node::ArrayBufferAllocator> allocator)
    : isolate_{isolate}, allocator_{std::move(allocator)} {
  RegisterIsolateScheduler(isolate_, this);

  uv_loop_ = std::make_unique<uv_loop_t>();
  uv_loop_init(uv_loop_.get());

  uv_task_ = std::make_unique<uv_async_t>();
  uv_async_init(uv_loop_.get(), uv_task_.get(), [](uv_async_t* handle) {
    auto* scheduler = static_cast<UVScheduler*>(handle->data);
    if (!scheduler->working_.load()) {
      scheduler->timer_manager_.reset();
      scheduler->working_.store(true, std::memory_order_relaxed);
      uv_stop(handle->loop);
      return;
    }

    if (const int ref_count{
            scheduler->uv_ref_count_.load(std::memory_order_acquire)};
        ref_count == 1) {
      uv_ref(reinterpret_cast<uv_handle_t*>(handle));
    } else if (ref_count == 0) {
      uv_unref(reinterpret_cast<uv_handle_t*>(handle));
    }

    scheduler->FlushForegroundTasks();
  });
  uv_task_->data = this;
}

UVScheduler<Scheduler::Type::kSelf>::~UVScheduler() {
  UnregisterIsolateScheduler(isolate_);

  working_.store(false, std::memory_order_release);
  uv_async_send(uv_task_.get());
  if (thread_.joinable()) {
    thread_.join();
  }
}

void UVScheduler<Scheduler::Type::kSelf>::PostTask(
    std::unique_ptr<v8::Task> task,
    const TaskType type) {
  switch (type) {
    case TaskType::kMacro: {
      std::lock_guard lock{mutex_macro_};
      tasks_macro_.push(std::move(task));
    } break;
    case TaskType::kMicro: {
      std::lock_guard lock{mutex_micro_};
      tasks_micro_.push(std::move(task));
    } break;
    case TaskType::kInterrupt: {
      std::lock_guard lock{mutex_interrupt_};
      tasks_interrupt_.push(std::move(task));
    } break;
    default:;
  }
  uv_async_send(uv_task_.get());
}

uint32_t UVScheduler<Scheduler::Type::kSelf>::PostDelayedTask(
    std::unique_ptr<v8::Task> task,
    uint64_t ms,
    Timer::Type type) {
  return timer_manager_->AddTimer(std::move(task), ms, type);
}

void UVScheduler<Scheduler::Type::kSelf>::StartLoop() {
  thread_ = std::thread{[this] {
    {
      v8::Locker locker{isolate_};
      v8::Isolate::Scope isolate_scope{isolate_};
      v8::HandleScope handle_scope{isolate_};

      isolate_data_ = node::CreateIsolateData(
          isolate_, uv_loop_.get(), PlatformDelegate::GetNodePlatform(),
          allocator_.get());
      timer_manager_ = std::make_unique<TimerManager>(uv_loop_.get());
      inspector_agent_ = std::make_unique<InspectorAgent>(isolate_, this);
      working_.store(true, std::memory_order_release);

      uv_run(uv_loop_.get(), UV_RUN_DEFAULT);
      inspector_agent_.reset();
    }

    node::FreeIsolateData(isolate_data_);
    PlatformDelegate::UnregisterIsolate(isolate_);
    isolate_->Dispose();

    uv_task_.reset();
    uv_loop_.reset();
  }};
}

void UVScheduler<Scheduler::Type::kSelf>::FlushMicroTasks() {
  while (true) {
    TaskQueue tasks_micro;
    {
      std::lock_guard lock{mutex_micro_};
      tasks_micro = std::exchange(tasks_micro_, {});
    }
    if (tasks_micro.empty()) {
      return;
    }

    {
      v8::HandleScope handle_scope{isolate_};
      while (!tasks_micro.empty()) {
        tasks_micro.front()->Run();
        tasks_micro.pop();
      }
      isolate_->PerformMicrotaskCheckpoint();
    }
  }
}
void UVScheduler<Scheduler::Type::kSelf>::FlushInterruptTasks() {
  while (true) {
    TaskQueue tasks_interrupt;
    {
      std::lock_guard lock{mutex_interrupt_};
      tasks_interrupt = std::exchange(tasks_interrupt_, {});
    }
    if (tasks_interrupt.empty()) {
      return;
    }

    {
      v8::HandleScope handle_scope{isolate_};
      while (!tasks_interrupt.empty()) {
        tasks_interrupt.front()->Run();
        tasks_interrupt.pop();
      }
    }
  }
}
void UVScheduler<Scheduler::Type::kSelf>::RunForegroundTask(
    std::unique_ptr<v8::Task> task) const {
  if (!working_.load()) {
    return;
  }

  v8::MicrotasksScope scope{isolate_, v8::MicrotasksScope::kRunMicrotasks};
  task->Run();
}
void UVScheduler<Scheduler::Type::kSelf>::FlushForegroundTasks() {
  FlushInterruptTasks();
  FlushMicroTasks();

  while (true) {
    TaskQueue tasks_macro;
    {
      std::lock_guard lock{mutex_macro_};
      tasks_macro = std::exchange(tasks_macro_, {});
    }
    if (tasks_macro.empty()) {
      return;
    }

    {
      v8::HandleScope handle_scope{isolate_};
      while (!tasks_macro.empty()) {
        std::unique_ptr<v8::Task> task = std::move(tasks_macro.front());
        tasks_macro.pop();
        RunForegroundTask(std::move(task));
        FlushInterruptTasks();
        FlushMicroTasks();
      }
    }
  }
}

void UVScheduler<Scheduler::Type::kSelf>::AgentConnect(const int port) const {
  inspector_agent_->Connect(port);
}
void UVScheduler<Scheduler::Type::kSelf>::AgentDisconnect() const {
  inspector_agent_->Disconnect();
}
void UVScheduler<Scheduler::Type::kSelf>::AgentAddContext(
    v8::Local<v8::Context> context,
    const std::string& name) const {
  inspector_agent_->AddContext(context, name);
}
void UVScheduler<Scheduler::Type::kSelf>::AgentDispatchProtocolMessage(
    std::string message) const {
  inspector_agent_->DispatchProtocolMessage(std::move(message));
}
void UVScheduler<Scheduler::Type::kSelf>::AgentDispose() const {
  inspector_agent_->Dispose();
}

void UVScheduler<Scheduler::Type::kSelf>::Ref() {
  if (++uv_ref_count_ == 1) {
    uv_async_send(uv_task_.get());
  }
}
void UVScheduler<Scheduler::Type::kSelf>::Unref() {
  if (--uv_ref_count_ == 0) {
    uv_async_send(uv_task_.get());
  }
}

}  // namespace svm
