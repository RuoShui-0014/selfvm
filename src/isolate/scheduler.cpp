#include "scheduler.h"

#include <uv.h>

#include <map>

#include "../utils/utils.h"
#include "platform_delegate.h"

namespace svm {
namespace {
std::mutex mutex;
std::map<v8::Isolate*, uv_loop_t*> isolate_map;
}  // namespace

void Scheduler::RegisterIsolate(v8::Isolate* isolate, uv_loop_t* loop) {
  std::lock_guard lock(mutex);
  isolate_map.emplace(isolate, loop);
}
uv_loop_t* Scheduler::GetIsolateUvLoop(v8::Isolate* isolate) {
  if (const auto it = isolate_map.find(isolate); it != isolate_map.end()) {
    return it->second;
  }
  return nullptr;
}

UVSchedulerSel::UVSchedulerSel(
    v8::Isolate* isolate,
    std::unique_ptr<node::ArrayBufferAllocator> allocator)
    : isolate_(isolate), allocator_(std::move(allocator)) {
  uv_loop_ = new uv_loop_t;
  uv_loop_init(uv_loop_);

  Scheduler::RegisterIsolate(isolate_, uv_loop_);

  keep_alive_ = new uv_async_t;
  uv_async_init(uv_loop_, keep_alive_, [](uv_async_t* handle) {
    UVSchedulerSel* scheduler = static_cast<UVSchedulerSel*>(handle->data);
    if (!scheduler->running.load()) {
      uv_stop(handle->loop);
      return;
    }

    InspectorLoop(scheduler);
  });
  keep_alive_->data = this;
}
UVSchedulerSel::~UVSchedulerSel() {
  running.store(false);
  uv_async_send(keep_alive_);
  if (thread_.joinable()) {
    thread_.join();
  }
}
void UVSchedulerSel::RunLoop() {
  thread_ = std::thread([this]() {
    {
      v8::Locker locker(isolate_);
      v8::Isolate::Scope isolate_scope(isolate_);
      v8::HandleScope handle_scope(isolate_);

      isolate_data_ = node::CreateIsolateData(
          isolate_, uv_loop_, PlatformDelegate::GetNodePlatform(),
          allocator_.get());

      running.store(true);
      uv_run(uv_loop_, UV_RUN_DEFAULT);
    }

    uv_close(reinterpret_cast<uv_handle_t*>(keep_alive_),
             [](uv_handle_t* handle) {
               delete reinterpret_cast<uv_async_t*>(handle);
             });
    uv_loop_close(uv_loop_);

    node::FreeIsolateData(isolate_data_);
    PlatformDelegate::UnregisterIsolate(isolate_);
    isolate_->Dispose();

    delete uv_loop_;
  });
}
void UVSchedulerSel::KeepAlive() {
  if (++uv_ref_count == 1) {
    uv_ref(reinterpret_cast<uv_handle_t*>(keep_alive_));
  }
}
void UVSchedulerSel::WillDie() {
  if (--uv_ref_count == 0) {
    uv_unref(reinterpret_cast<uv_handle_t*>(keep_alive_));
  }
}
std::shared_ptr<v8::TaskRunner> UVSchedulerSel::TaskRunner() const {
  if (isolate_) {
    return PlatformDelegate::GetNodePlatform()->GetForegroundTaskRunner(
        isolate_);
  }
  return {};
}

void UVSchedulerSel::InspectorLoop(UVSchedulerSel* scheduler) {
  std::queue<std::unique_ptr<v8::Task>> tasks;
  {
    std::lock_guard lock(scheduler->mutex_);
    tasks = std::exchange(scheduler->tasks_, {});
  }

  while (!tasks.empty()) {
    tasks.front()->Run();
    tasks.pop();
  }
}

void UVSchedulerSel::PostInspectorTask(std::unique_ptr<v8::Task> task) {
  {
    std::lock_guard lock(mutex_);
    tasks_.push(std::move(task));
  }
  uv_async_send(keep_alive_);
}

UVSchedulerPar::UVSchedulerPar(v8::Isolate* isolate) : isolate_(isolate) {
  uv_loop_ = Scheduler::GetIsolateUvLoop(isolate_);
  keep_alive_ = new uv_async_t;
  uv_async_init(uv_loop_, keep_alive_, [](uv_async_t*) {});
  uv_unref(reinterpret_cast<uv_handle_t*>(keep_alive_));
}
UVSchedulerPar::~UVSchedulerPar() {
  uv_close(reinterpret_cast<uv_handle_t*>(keep_alive_),
           [](uv_handle_t* handle) {
             delete reinterpret_cast<uv_async_t*>(handle);
           });
}
void UVSchedulerPar::KeepAlive() {
  if (++uv_ref_count == 1) {
    uv_ref(reinterpret_cast<uv_handle_t*>(keep_alive_));
  }
}
void UVSchedulerPar::WillDie() {
  if (--uv_ref_count == 0) {
    uv_unref(reinterpret_cast<uv_handle_t*>(keep_alive_));
  }
}
std::shared_ptr<v8::TaskRunner> UVSchedulerPar::TaskRunner() const {
  if (isolate_) {
    return PlatformDelegate::GetNodePlatform()->GetForegroundTaskRunner(
        isolate_);
  }
  return {};
}

}  // namespace svm
