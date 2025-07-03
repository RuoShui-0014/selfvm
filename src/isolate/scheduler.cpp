#include "scheduler.h"

namespace svm {

Scheduler::Scheduler() {
  // thread_ = std::thread(&Scheduler::Entry, this);
}
Scheduler::~Scheduler() {}

void Scheduler::PostTask(std::unique_ptr<v8::Task> task) {
  {
    std::lock_guard lock(queue_mutex_);
    tasks_.push(std::move(task));
  }
  cv_.notify_one();
}

void Scheduler::PostDelayedTask(std::unique_ptr<v8::Task> task,
                                double delay_in_seconds) {
  {
    std::lock_guard lock(queue_mutex_);
    tasks_.push(std::move(task));
  }
  cv_.notify_one();
}
void Scheduler::PostNonNestableTask(std::unique_ptr<v8::Task> task) {
  {
    std::lock_guard lock(queue_mutex_);
    tasks_.push(std::move(task));
  }
  cv_.notify_one();
}

template <class Type>
auto ExchangeDefault(Type& container) {
  return std::exchange(container, Type{});
}
void Scheduler::RunTask() {
  while (true) {
    TaskQueue tasks;
    TaskQueue handle_tasks;
    TaskQueue interrupts;
    {
      // Grab current tasks
      auto lock = Lock();
      tasks = ExchangeDefault(lock->tasks_);
      handle_tasks = ExchangeDefault(lock->handle_tasks_);
      interrupts = ExchangeDefault(lock->interrupts_);
      if (tasks.empty() && handle_tasks.empty() && interrupts.empty()) {
        // lock->DoneRunning();
        return;
      }
    }

    // Execute interrupt tasks
    while (!interrupts.empty()) {
      interrupts.front()->Run();
      interrupts.pop();
    }

    // Execute handle tasks
    while (!handle_tasks.empty()) {
      handle_tasks.front()->Run();
      handle_tasks.pop();
    }

    // Execute tasks
    while (!tasks.empty()) {
      tasks.front()->Run();
      tasks.pop();
    }
  }
}

[[noreturn]] void Scheduler::Entry() {
  while (true) {
    {
      std::unique_lock lock(exec_mutex_);
      cv_.wait_for(lock, std::chrono::milliseconds(10),
                   [&] { return !tasks_.empty(); });
    }
    RunTask();
  }
}

}  // namespace svm
