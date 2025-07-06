#include "scheduler.h"

#include <iostream>

#include "../utils/utils.h"
#include "uv.h"

namespace svm {

Scheduler::Scheduler(v8::Isolate* isolate, uv_loop_t* loop)
    : isolate_(isolate), uv_loop_(loop), flush_(new uv_async_t) {
  if (!loop) {
    uv_loop_ = new uv_loop_t;
    uv_loop_init(uv_loop_);

    keep_alive_ = new uv_idle_t;
    uv_idle_init(uv_loop_, keep_alive_);
    uv_idle_start(keep_alive_, [](uv_idle_t* handle) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    });

    thread_ = std::thread([=]() {
      uv_run(uv_loop_, UV_RUN_DEFAULT);

      uv_idle_stop(keep_alive_);
      uv_close(reinterpret_cast<uv_handle_t*>(keep_alive_),
               [](uv_handle_t* handle) {
                 delete reinterpret_cast<uv_idle_t*>(handle);
               });

      uv_loop_close(uv_loop_);
      delete uv_loop_;
    });
  }

  uv_async_init(uv_loop_, flush_, &Scheduler::RunTask);
  flush_->data = this;
  uv_unref(reinterpret_cast<uv_handle_t*>(flush_));
}
Scheduler::~Scheduler() {
  if (keep_alive_) {
    uv_stop(uv_loop_);
    if (thread_.joinable()) {
      thread_.join();
    }
    uv_close(reinterpret_cast<uv_handle_t*>(flush_), [](uv_handle_t* handle) {
      delete reinterpret_cast<uv_async_t*>(handle);
    });
    return;
  }

  uv_close(reinterpret_cast<uv_handle_t*>(flush_), [](uv_handle_t* handle) {
    delete reinterpret_cast<uv_async_t*>(handle);
  });
}

void Scheduler::PostTask(std::unique_ptr<v8::Task> task) {
  {
    std::lock_guard lock(mutex_);
    tasks_.push(std::move(task));
  }
  uv_async_send(flush_);
}

void Scheduler::PostDelayedTask(std::unique_ptr<v8::Task> task,
                                double delay_in_seconds) {
  {
    std::lock_guard lock(mutex_);
    tasks_.push(std::move(task));
  }
  uv_async_send(flush_);
}

void Scheduler::PostNonNestableTask(std::unique_ptr<v8::Task> task) {
  {
    std::lock_guard lock(mutex_);
    tasks_.push(std::move(task));
  }
  uv_async_send(flush_);
}

void Scheduler::RunTask(uv_async_t* flush) {
  auto& scheduler = *static_cast<Scheduler*>(flush->data);
  while (true) {
    TaskQueue tasks;
    TaskQueue handle_tasks;
    TaskQueue interrupts;
    {
      // Grab current tasks
      auto lock = scheduler.Lock();
      tasks = std::exchange(scheduler.tasks_, {});
      handle_tasks = std::exchange(scheduler.handle_tasks_, {});
      interrupts = std::exchange(scheduler.interrupts_, {});
      if (tasks.empty() && handle_tasks.empty() && interrupts.empty()) {
        return;
      }
    }

    while (!interrupts.empty()) {
      interrupts.front()->Run();
      interrupts.pop();
    }

    while (!handle_tasks.empty()) {
      handle_tasks.front()->Run();
      handle_tasks.pop();
    }

    while (!tasks.empty()) {
      tasks.front()->Run();
      tasks.pop();
    }
  }
}

void Scheduler::Send() const {
  uv_async_send(flush_);
}

}  // namespace svm
