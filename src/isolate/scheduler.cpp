#include "scheduler.h"

#include <uv.h>

#include <map>

#include "base/logger.h"
#include "isolate/inspector_agent.h"
#include "isolate/platform_delegate.h"
#include "isolate_holder.h"
#include "native/timer_manager.h"

namespace svm {

namespace {
std::mutex mutex;
std::map<v8::Isolate*, Scheduler*> g_isolate_scheduler_map;
}  // namespace

void Scheduler::RegisterIsolateScheduler(v8::Isolate* isolate,
                                         Scheduler* scheduler) {
  std::lock_guard lock{mutex};
  g_isolate_scheduler_map.emplace(isolate, scheduler);
}
void Scheduler::UnregisterIsolateScheduler(v8::Isolate* isolate) {
  std::lock_guard lock{mutex};
  g_isolate_scheduler_map.erase(isolate);
}
Scheduler* Scheduler::GetIsolateScheduler(v8::Isolate* isolate) {
  if (const auto it{g_isolate_scheduler_map.find(isolate)};
      it != g_isolate_scheduler_map.end()) {
    return it->second;
  }
  return nullptr;
}

UVScheduler::UVScheduler(v8::Isolate* isolate) : isolate_{isolate} {
  RegisterIsolateScheduler(isolate_, this);
}
UVScheduler::~UVScheduler() {
  UnregisterIsolateScheduler(isolate_);
}

std::shared_ptr<v8::TaskRunner> UVScheduler::TaskRunner() {
  if (task_runner_) {
    return task_runner_;
  }

  task_runner_ =
      PlatformDelegate::GetNodePlatform()->GetForegroundTaskRunner(isolate_);
  return task_runner_;
}

void UVScheduler::PostTask(std::unique_ptr<v8::Task> task, TaskType type) {
  TaskRunner()->PostTask(std::move(task));
}
uint32_t UVScheduler::PostDelayedTask(std::unique_ptr<v8::Task> task,
                                      uint64_t ms,
                                      Timer::Type type) {
  TaskRunner()->PostDelayedTask(std::move(task), ms / 1000.0);
  return 0;
}

void UVScheduler::Ref() {
  if (++uv_ref_count_ == 1) {
    uv_async_send(uv_task_);
  }
}
void UVScheduler::Unref() {
  if (--uv_ref_count_ == 0) {
    uv_async_send(uv_task_);
  }
}

UVSchedulerSel::UVSchedulerSel(
    v8::Isolate* isolate,
    std::unique_ptr<node::ArrayBufferAllocator> allocator)
    : UVScheduler{isolate}, allocator_{std::move(allocator)} {
  uv_loop_ = new uv_loop_t{};
  uv_loop_init(uv_loop_);

  uv_task_ = new uv_async_t{};
  uv_async_init(uv_loop_, uv_task_, [](uv_async_t* handle) {
    auto* scheduler = static_cast<UVSchedulerSel*>(handle->data);
    if (!scheduler->running_.load()) {
      scheduler->timer_manager_.reset();
      scheduler->running_.store(true, std::memory_order_relaxed);
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

    scheduler->FlushForegroundTasksInternal();
  });
  uv_task_->data = this;
}
UVSchedulerSel::~UVSchedulerSel() {
  running_.store(false, std::memory_order_release);
  uv_async_send(uv_task_);
  if (thread_.joinable()) {
    thread_.join();
  }
}
void UVSchedulerSel::PostTask(std::unique_ptr<v8::Task> task,
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
  uv_async_send(uv_task_);
}

uint32_t UVSchedulerSel::PostDelayedTask(std::unique_ptr<v8::Task> task,
                                         uint64_t ms,
                                         Timer::Type type) {
  return timer_manager_->AddTimer(std::move(task), ms, type);
}

void UVSchedulerSel::FlushMicroTasks() {
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
void UVSchedulerSel::FlushInterruptTasks() {
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
void UVSchedulerSel::RunForegroundTask(std::unique_ptr<v8::Task> task) const {
  if (!running_.load()) {
    return;
  }

  v8::MicrotasksScope scope{isolate_, v8::MicrotasksScope::kRunMicrotasks};
  task->Run();
}
void UVSchedulerSel::FlushForegroundTasksInternal() {
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

void UVSchedulerSel::AgentConnect(const int port) const {
  inspector_agent_->Connect(port);
}
void UVSchedulerSel::AgentDisconnect() const {
  inspector_agent_->Disconnect();
}
void UVSchedulerSel::AgentAddContext(v8::Local<v8::Context> context,
                                     const std::string& name) const {
  inspector_agent_->AddContext(context, name);
}
void UVSchedulerSel::AgentDispatchProtocolMessage(std::string message) const {
  inspector_agent_->DispatchProtocolMessage(std::move(message));
}
void UVSchedulerSel::AgentDispose() const {
  inspector_agent_->Dispose();
}

void UVSchedulerSel::StartLoop() {
  thread_ = std::thread{[this] {
    {
      v8::Locker locker{isolate_};
      v8::Isolate::Scope isolate_scope{isolate_};
      v8::HandleScope handle_scope{isolate_};

      isolate_data_ = node::CreateIsolateData(
          isolate_, uv_loop_, PlatformDelegate::GetNodePlatform(),
          allocator_.get());
      timer_manager_ = std::make_unique<TimerManager>(uv_loop_);
      inspector_agent_ = std::make_unique<InspectorAgent>(isolate_, this);
      running_.store(true, std::memory_order_release);

      uv_run(uv_loop_, UV_RUN_DEFAULT);
      inspector_agent_.reset();
    }

    node::FreeIsolateData(isolate_data_);
    PlatformDelegate::UnregisterIsolate(isolate_);
    isolate_->Dispose();

    uv_close(reinterpret_cast<uv_handle_t*>(uv_task_), [](uv_handle_t* handle) {
      delete reinterpret_cast<uv_async_t*>(handle);
    });
    uv_loop_close(uv_loop_);
    delete uv_loop_;
  }};
}

UVSchedulerPar* UVSchedulerPar::nodejs_scheduler{nullptr};
UVSchedulerPar::UVSchedulerPar(v8::Isolate* isolate, uv_loop_t* uv_loop)
    : UVScheduler{isolate} {
  nodejs_scheduler = this;

  uv_loop_ = uv_loop;
  uv_task_ = new uv_async_t{};
  uv_async_init(uv_loop_, uv_task_, [](uv_async_t* handle) {
    auto* scheduler = static_cast<UVSchedulerPar*>(handle->data);
    if (const int ref_count{
            scheduler->uv_ref_count_.load(std::memory_order_acquire)};
        ref_count == 1) {
      uv_ref(reinterpret_cast<uv_handle_t*>(handle));
    } else if (ref_count == 0) {
      uv_unref(reinterpret_cast<uv_handle_t*>(handle));
    }

    scheduler->FlushForegroundTasksInternal();
  });
  uv_task_->data = this;
  UVScheduler::Unref();

  timer_manager_ = std::make_unique<TimerManager>(uv_loop_);
}
UVSchedulerPar::~UVSchedulerPar() {
  timer_manager_.reset();
  uv_close(reinterpret_cast<uv_handle_t*>(uv_task_), [](uv_handle_t* handle) {
    delete reinterpret_cast<uv_async_t*>(handle);
  });
}
void UVSchedulerPar::PostTask(std::unique_ptr<v8::Task> task,
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
  uv_async_send(uv_task_);
}

void UVSchedulerPar::FlushMicroTasks() {
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
void UVSchedulerPar::FlushInterruptTasks() {
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
void UVSchedulerPar::RunForegroundTask(std::unique_ptr<v8::Task> task) const {
  if (!running_.load()) {
    return;
  }

  v8::MicrotasksScope scope{isolate_, v8::MicrotasksScope::kRunMicrotasks};
  task->Run();
}
void UVSchedulerPar::FlushForegroundTasksInternal() {
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

}  // namespace svm
