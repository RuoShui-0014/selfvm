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

UVScheduler::UVScheduler() = default;
UVScheduler::UVScheduler(v8::Isolate* isolate, uv_loop_t* loop)
    : isolate_(isolate), uv_loop_(loop) {
  if ((is_self_ = !loop)) {
    uv_loop_ = new uv_loop_t;
    uv_loop_init(uv_loop_);

    keep_alive_ = new uv_async_t;
    uv_async_init(uv_loop_, keep_alive_,
                  [](uv_async_t* handle) { uv_stop(handle->loop); });

    thread_ = std::thread([=]() {
      uv_run(uv_loop_, UV_RUN_DEFAULT);

      uv_close(reinterpret_cast<uv_handle_t*>(keep_alive_),
               [](uv_handle_t* handle) {
                 delete reinterpret_cast<uv_async_t*>(handle);
               });
      uv_loop_close(uv_loop_);
      delete uv_loop_;
    });
  } else {
    keep_alive_ = new uv_async_t;
    uv_async_init(uv_loop_, keep_alive_,
                  [](uv_async_t* handle) { uv_stop(handle->loop); });
    uv_unref(reinterpret_cast<uv_handle_t*>(keep_alive_));
  }
}

UVScheduler::~UVScheduler() {
  if (is_self_) {
    uv_async_send(keep_alive_);
    if (thread_.joinable()) {
      thread_.join();
    }
  } else {
    uv_close(reinterpret_cast<uv_handle_t*>(keep_alive_),
             [](uv_handle_t* handle) {
               delete reinterpret_cast<uv_async_t*>(handle);
             });
  }
}

std::shared_ptr<v8::TaskRunner> UVScheduler::TaskRunner() const {
  return PlatformDelegate::GetNodePlatform()->GetForegroundTaskRunner(isolate_);
}

void UVScheduler::KeepAlive() {
  if (++uv_ref_count == 1) {
    uv_ref(reinterpret_cast<uv_handle_t*>(keep_alive_));
  }
}

void UVScheduler::WillDie() {
  if (--uv_ref_count == 0) {
    uv_unref(reinterpret_cast<uv_handle_t*>(keep_alive_));
  }
}

}  // namespace svm
