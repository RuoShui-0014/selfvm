#include "scheduler.h"

#include <uv.h>

#include <map>

#include "../utils/utils.h"
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
Scheduler* Scheduler::GetIsolateScheduler(v8::Isolate* const isolate) {
  const auto it{isolate_loop_map.find(isolate)};
  if (it != isolate_loop_map.end()) {
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

void Scheduler::PostTask(std::unique_ptr<v8::Task> task) {
  {
    std::lock_guard lock{mutex_task_};
    tasks_.push(std::move(task));
  }
  uv_async_send(uv_task_);
}
void Scheduler::PostDelayedTask(std::unique_ptr<v8::Task> task, double delay) {
  {
    std::lock_guard lock{mutex_task_};
    tasks_.push(std::move(task));
  }
  uv_async_send(uv_task_);
}
void Scheduler::PostHandleTask(std::unique_ptr<v8::Task> task) {
  {
    std::lock_guard lock{mutex_task_};
    tasks_handle_.push(std::move(task));
  }
  uv_async_send(uv_task_);
}
void Scheduler::PostInterruptTask(std::unique_ptr<v8::Task> task) {
  {
    std::lock_guard lock{mutex_task_};
    tasks_interrupts_.push(std::move(task));
  }
  uv_async_send(uv_task_);
}

void Scheduler::RunForegroundTask(std::unique_ptr<v8::Task> task) const {
  if (!running.load()) {
    return;
  }
  task->Run();
}

void Scheduler::FlushForegroundTasksInternal() {
  while (true) {
    TaskQueue tasks, tasks_handle, tasks_interrupts;
    {
      std::lock_guard lock{mutex_task_};
      tasks = std::exchange(tasks_, {});
      tasks_handle = std::exchange(tasks_handle_, {});
      tasks_interrupts = std::exchange(tasks_interrupts_, {});
    }
    if (tasks.empty() && tasks_handle.empty() && tasks_interrupts.empty()) {
      return;
    }

    {
      v8::HandleScope handle_scope{isolate_};

      //
      while (!tasks_interrupts.empty()) {
        tasks_interrupts.front()->Run();
        tasks_interrupts.pop();
      }

      // 处理微任务句柄
      while (!tasks_handle.empty()) {
        tasks_handle.front()->Run();
        tasks_handle.pop();
      }

      // 执行宏任务
      while (!tasks.empty()) {
        std::unique_ptr<v8::Task> task = std::move(tasks.front());
        tasks.pop();
        RunForegroundTask(std::move(task));
      }
    }
  }
}
void Scheduler::KeepAlive() {
  if (++uv_ref_count == 1) {
    uv_ref(reinterpret_cast<uv_handle_t*>(uv_task_));
  }
}
void Scheduler::WillDie() {
  if (--uv_ref_count == 0) {
    uv_unref(reinterpret_cast<uv_handle_t*>(uv_task_));
  }
}

UVSchedulerSel::UVSchedulerSel(
    v8::Isolate* isolate,
    std::unique_ptr<node::ArrayBufferAllocator> allocator)
    : Scheduler{isolate}, allocator_{std::move(allocator)} {
  uv_loop_ = new uv_loop_t;
  uv_loop_init(uv_loop_);

  uv_task_ = new uv_async_t;
  uv_async_init(uv_loop_, uv_task_, [](uv_async_t* handle) {
    UVSchedulerSel* scheduler = static_cast<UVSchedulerSel*>(handle->data);
    if (!scheduler->running.load()) {
      uv_stop(handle->loop);
      return;
    }

    scheduler->FlushForegroundTasksInternal();
  });
  uv_task_->data = this;
}
UVSchedulerSel::~UVSchedulerSel() {
  running.store(false);
  uv_async_send(uv_task_);
  if (thread_.joinable()) {
    thread_.join();
  }
}
void UVSchedulerSel::AgentConnect(int port) const {
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
  thread_ = std::thread([this]() {
    {
      v8::Locker locker{isolate_};
      v8::Isolate::Scope isolate_scope{isolate_};
      v8::HandleScope handle_scope{isolate_};

      isolate_data_ = node::CreateIsolateData(
          isolate_, uv_loop_, PlatformDelegate::GetNodePlatform(),
          allocator_.get());
      inspector_agent_ = std::make_unique<InspectorAgent>(isolate_, this);

      running.store(true);
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
  });
}

void UVSchedulerSel::RunInterruptTasks() {
  TaskQueue tasks;
  {
    std::lock_guard lock{mutex_task_};
    tasks = std::exchange(tasks_interrupts_, {});
  }

  while (!tasks.empty()) {
    tasks.front()->Run();
    tasks.pop();
  }
}

UVSchedulerPar* UVSchedulerPar::nodejs_scheduler{};

UVSchedulerPar::UVSchedulerPar(v8::Isolate* isolate, uv_loop_t* uv_loop)
    : Scheduler{isolate} {
  nodejs_scheduler = this;

  uv_loop_ = uv_loop;
  uv_task_ = new uv_async_t;
  uv_async_init(uv_loop_, uv_task_, [](uv_async_t* handle) {
    UVSchedulerPar* scheduler = static_cast<UVSchedulerPar*>(handle->data);
    scheduler->FlushForegroundTasksInternal();
  });
  uv_task_->data = this;
  uv_unref(reinterpret_cast<uv_handle_t*>(uv_task_));
}
UVSchedulerPar::~UVSchedulerPar() {
  uv_close(reinterpret_cast<uv_handle_t*>(uv_task_), [](uv_handle_t* handle) {
    delete reinterpret_cast<uv_async_t*>(handle);
  });
}

}  // namespace svm
