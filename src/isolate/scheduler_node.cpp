#include "scheduler_node.h"

namespace svm {

UVScheduler<Scheduler::Type::kNode>::UVScheduler(v8::Isolate* isolate,
                                                 uv_loop_t* uv_loop)
    : isolate_{isolate} {
  RegisterIsolateScheduler(isolate_, this);
  node_scheduler = this;

  uv_loop_ = std::unique_ptr<uv_loop_t>(uv_loop);
  uv_task_ = std::make_unique<uv_async_t>();
  uv_async_init(uv_loop_.get(), uv_task_.get(), [](uv_async_t* handle) {
    auto* scheduler = static_cast<UVScheduler*>(handle->data);
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
  UVScheduler::Unref();

  timer_manager_ = std::make_unique<TimerManager>(uv_loop_.get());
}
UVScheduler<Scheduler::Type::kNode>::~UVScheduler() {
  node_scheduler = nullptr;
  timer_manager_.reset();
  uv_task_.reset();
  uv_loop_.release();
}

void UVScheduler<Scheduler::Type::kNode>::PostTask(
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

uint32_t UVScheduler<Scheduler::Type::kNode>::PostDelayedTask(
    std::unique_ptr<v8::Task> task,
    uint64_t ms,
    Timer::Type type) {
  return timer_manager_->AddTimer(std::move(task), ms, type);
}

void UVScheduler<Scheduler::Type::kNode>::FlushMicroTasks() {
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
void UVScheduler<Scheduler::Type::kNode>::FlushInterruptTasks() {
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
void UVScheduler<Scheduler::Type::kNode>::RunForegroundTask(
    std::unique_ptr<v8::Task> task) const {
  v8::MicrotasksScope scope{isolate_, v8::MicrotasksScope::kRunMicrotasks};
  task->Run();
}
void UVScheduler<Scheduler::Type::kNode>::FlushForegroundTasks() {
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

void UVScheduler<Scheduler::Type::kNode>::Ref() {
  if (++uv_ref_count_ == 1) {
    uv_async_send(uv_task_.get());
  }
}
void UVScheduler<Scheduler::Type::kNode>::Unref() {
  if (--uv_ref_count_ == 0) {
    uv_async_send(uv_task_.get());
  }
}

}  // namespace svm
