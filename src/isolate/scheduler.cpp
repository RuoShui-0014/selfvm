#include "scheduler.h"

#include <uv.h>

#include <map>

#include "../base/logger.h"
#include "../native/timer_manager.h"
#include "inspector_agent.h"
#include "platform_delegate.h"

namespace svm {

namespace {
std::mutex mutex;
std::map<v8::Isolate*, Scheduler*> isolate_loop_map;
}  // namespace

Scheduler::Scheduler(v8::Isolate* isolate) : isolate_{isolate} {
  RegisterIsolateScheduler(isolate_, this);
}
Scheduler::~Scheduler() {
  UnregisterIsolateScheduler(isolate_);
}

void Scheduler::RegisterIsolateScheduler(v8::Isolate* isolate,
                                         Scheduler* loop) {
  std::lock_guard lock{mutex};
  isolate_loop_map.emplace(isolate, loop);
}
void Scheduler::UnregisterIsolateScheduler(v8::Isolate* isolate) {
  std::lock_guard lock{mutex};
  isolate_loop_map.erase(isolate);
}
Scheduler* Scheduler::GetIsolateScheduler(v8::Isolate* isolate) {
  if (const auto it{isolate_loop_map.find(isolate)};
      it != isolate_loop_map.end()) {
    return it->second;
  }
  return nullptr;
}

std::shared_ptr<v8::TaskRunner> Scheduler::TaskRunner() {
  if (task_runner_) {
    return task_runner_;
  }

  task_runner_ =
      PlatformDelegate::GetNodePlatform()->GetForegroundTaskRunner(isolate_);
  return task_runner_;
}

void Scheduler::PostMacroTask(std::unique_ptr<v8::Task> task) {
  TaskRunner()->PostTask(std::move(task));
}
void Scheduler::PostDelayedTask(std::unique_ptr<v8::Task> task, uint64_t ms) {
  TaskRunner()->PostDelayedTask(std::move(task), ms / 1000.0);
}
void Scheduler::PostMicroTask(std::unique_ptr<v8::Task> task) {
  TaskRunner()->PostTask(std::move(task));
}
void Scheduler::PostInterruptTask(std::unique_ptr<v8::Task> task) {
  TaskRunner()->PostTask(std::move(task));
}

void Scheduler::FlushForegroundTasksInternal() {
  PlatformDelegate::GetNodePlatform()->FlushForegroundTasks(isolate_);
}

void Scheduler::Ref() {
  if (++uv_ref_count == 1) {
    uv_ref(reinterpret_cast<uv_handle_t*>(uv_task_));
  }
}
void Scheduler::Unref() {
  if (--uv_ref_count == 0) {
    uv_unref(reinterpret_cast<uv_handle_t*>(uv_task_));
    uv_async_send(uv_task_);
  }
}

UVSchedulerSel::UVSchedulerSel(
    v8::Isolate* isolate,
    std::unique_ptr<node::ArrayBufferAllocator> allocator)
    : Scheduler{isolate}, allocator_{std::move(allocator)} {
  uv_loop_ = new uv_loop_t{};
  uv_loop_init(uv_loop_);

  uv_task_ = new uv_async_t{};
  uv_async_init(uv_loop_, uv_task_, [](uv_async_t* handle) {
    auto* scheduler = static_cast<UVSchedulerSel*>(handle->data);
    if (!scheduler->running_.load()) {
      scheduler->timer_manager_.reset();
      uv_stop(handle->loop);
      return;
    }

    scheduler->FlushForegroundTasksInternal();
  });
  uv_task_->data = this;
}
UVSchedulerSel::~UVSchedulerSel() {
  running_.store(false);
  uv_async_send(uv_task_);
  if (thread_.joinable()) {
    thread_.join();
  }
}

void UVSchedulerSel::PostMacroTask(std::unique_ptr<v8::Task> task) {
  {
    std::lock_guard lock{mutex_task_};
    tasks_macro_.push(std::move(task));
  }
  uv_async_send(uv_task_);
}
void UVSchedulerSel::PostMicroTask(std::unique_ptr<v8::Task> task) {
  {
    std::lock_guard lock{mutex_task_};
    tasks_micro_.push(std::move(task));
  }
  uv_async_send(uv_task_);
}
void UVSchedulerSel::PostInterruptTask(std::unique_ptr<v8::Task> task) {
  {
    std::lock_guard lock{mutex_task_};
    tasks_interrupt_.push(std::move(task));
  }
  uv_async_send(uv_task_);
}

void UVSchedulerSel::RunForegroundTask(std::unique_ptr<v8::Task> task) const {
  if (!running_.load()) {
    return;
  }
  task->Run();
}
void UVSchedulerSel::FlushForegroundTasksInternal() {
  while (true) {
    TaskQueue tasks_macro, tasks_micro, tasks_interrupt;
    {
      std::lock_guard lock{mutex_task_};
      tasks_macro = std::exchange(tasks_macro_, {});
      tasks_micro = std::exchange(tasks_micro_, {});
      tasks_interrupt = std::exchange(tasks_interrupt_, {});
    }
    if (tasks_macro.empty() && tasks_micro.empty() && tasks_interrupt.empty()) {
      return;
    }

    {
      v8::HandleScope handle_scope{isolate_};

      while (!tasks_interrupt.empty()) {
        tasks_interrupt.front()->Run();
        tasks_interrupt.pop();
      }

      // 处理微任务句柄
      while (!tasks_micro.empty()) {
        tasks_micro.front()->Run();
        tasks_micro.pop();
      }

      // 执行宏任务
      while (!tasks_macro.empty()) {
        std::unique_ptr<v8::Task> task = std::move(tasks_macro.front());
        tasks_macro.pop();
        RunForegroundTask(std::move(task));
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
  thread_ = std::thread{[this]() {
    {
      v8::Locker locker{isolate_};
      v8::Isolate::Scope isolate_scope{isolate_};
      v8::HandleScope handle_scope{isolate_};

      isolate_data_ = node::CreateIsolateData(
          isolate_, uv_loop_, PlatformDelegate::GetNodePlatform(),
          allocator_.get());
      timer_manager_ = std::make_unique<TimerManager>(uv_loop_);
      inspector_agent_ = std::make_unique<InspectorAgent>(isolate_, this);
      running_.store(true);

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
void UVSchedulerSel::RunInterruptTasks() {
  TaskQueue tasks;
  {
    std::lock_guard lock{mutex_task_};
    tasks = std::exchange(tasks_interrupt_, {});
  }

  while (!tasks.empty()) {
    tasks.front()->Run();
    tasks.pop();
  }
}

UVSchedulerPar* UVSchedulerPar::nodejs_scheduler{nullptr};
UVSchedulerPar::UVSchedulerPar(v8::Isolate* isolate, uv_loop_t* uv_loop)
    : Scheduler{isolate} {
  nodejs_scheduler = this;

  uv_loop_ = uv_loop;
  uv_task_ = new uv_async_t{};
  uv_async_init(uv_loop_, uv_task_, [](uv_async_t* handle) {
    auto* scheduler = static_cast<UVSchedulerPar*>(handle->data);
    scheduler->FlushForegroundTasksInternal();
  });
  uv_task_->data = this;
  uv_unref(reinterpret_cast<uv_handle_t*>(uv_task_));

  timer_manager_ = std::make_unique<TimerManager>(uv_loop_);
}
UVSchedulerPar::~UVSchedulerPar() {
  timer_manager_.reset();
  uv_close(reinterpret_cast<uv_handle_t*>(uv_task_), [](uv_handle_t* handle) {
    delete reinterpret_cast<uv_async_t*>(handle);
  });
}

void UVSchedulerPar::PostMacroTask(std::unique_ptr<v8::Task> task) {
  {
    std::lock_guard lock{mutex_task_};
    tasks_macro_.push(std::move(task));
  }
  uv_async_send(uv_task_);
}
void UVSchedulerPar::PostMicroTask(std::unique_ptr<v8::Task> task) {
  {
    std::lock_guard lock{mutex_task_};
    tasks_micro_.push(std::move(task));
  }
  uv_async_send(uv_task_);
}
void UVSchedulerPar::PostInterruptTask(std::unique_ptr<v8::Task> task) {
  {
    std::lock_guard lock{mutex_task_};
    tasks_interrupt_.push(std::move(task));
  }
  uv_async_send(uv_task_);
}

void UVSchedulerPar::RunForegroundTask(std::unique_ptr<v8::Task> task) const {
  if (!running.load()) {
    return;
  }
  task->Run();
}
void UVSchedulerPar::FlushForegroundTasksInternal() {
  while (true) {
    TaskQueue tasks_macro, tasks_micro, tasks_interrupt;
    {
      std::lock_guard lock{mutex_task_};
      tasks_macro = std::exchange(tasks_macro_, {});
      tasks_micro = std::exchange(tasks_micro_, {});
      tasks_interrupt = std::exchange(tasks_interrupt_, {});
    }
    if (tasks_macro.empty() && tasks_micro.empty() && tasks_interrupt.empty()) {
      return;
    }

    {
      v8::HandleScope handle_scope{isolate_};

      // 中断任务
      while (!tasks_interrupt.empty()) {
        tasks_interrupt.front()->Run();
        tasks_interrupt.pop();
      }

      // 处理微任务句柄
      while (!tasks_micro.empty()) {
        tasks_micro.front()->Run();
        tasks_micro.pop();
      }

      // 执行宏任务
      while (!tasks_macro.empty()) {
        std::unique_ptr<v8::Task> task = std::move(tasks_macro.front());
        tasks_macro.pop();
        RunForegroundTask(std::move(task));
      }
    }
  }
}

}  // namespace svm
