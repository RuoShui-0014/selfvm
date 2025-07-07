#include "scheduler.h"

#include <iostream>

#include "../utils/utils.h"
#include "platform_delegate.h"
#include "uv.h"

namespace svm {

IsolateScheduler::IsolateScheduler() = default;
IsolateScheduler::~IsolateScheduler() = default;

void IsolateScheduler::PostTask(std::unique_ptr<v8::Task> task) {}

void IsolateScheduler::PostDelayedTask(std::unique_ptr<v8::Task> task,
                                       double delay_in_seconds) {}

void IsolateScheduler::PostNonNestableTask(std::unique_ptr<v8::Task> task) {}


UVScheduler::UVScheduler(v8::Isolate* isolate, uv_loop_t* loop)
    : isolate_(isolate), uv_loop_(loop) {
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
}

UVScheduler::~UVScheduler() {
  if (keep_alive_) {
    uv_stop(uv_loop_);
    if (thread_.joinable()) {
      thread_.join();
    }
  }
}

std::shared_ptr<v8::TaskRunner> UVScheduler::TaskRunner() const {
  return PlatformDelegate::GetNodePlatform()->GetForegroundTaskRunner(isolate_);
}
}  // namespace svm
