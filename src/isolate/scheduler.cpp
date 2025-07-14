#include "scheduler.h"

#include <uv.h>

#include <map>

#include "../utils/utils.h"
#include "platform_delegate.h"

namespace svm {
namespace {
std::mutex mutex;
std::map<const v8::Isolate*, uv_loop_t*> isolate_loop_map;
}  // namespace

void Scheduler::RegisterIsolate(v8::Isolate* isolate, uv_loop_t* loop) {
  std::lock_guard lock(mutex);
  isolate_loop_map.emplace(isolate, loop);
}
uv_loop_t* Scheduler::GetIsolateUvLoop(const v8::Isolate* isolate) {
  const auto it = isolate_loop_map.find(isolate);
  if (it != isolate_loop_map.end()) {
    return it->second;
  }
  return nullptr;
}
std::shared_ptr<v8::TaskRunner> Scheduler::TaskRunner() const {
  if (isolate_) {
    return PlatformDelegate::GetNodePlatform()->GetForegroundTaskRunner(
        isolate_);
  }
  return {};
}

UVSchedulerSel::UVSchedulerSel(
    v8::Isolate* isolate,
    std::unique_ptr<node::ArrayBufferAllocator> allocator)
    : Scheduler(isolate), allocator_(std::move(allocator)) {
  uv_loop_ = new uv_loop_t;
  uv_loop_init(uv_loop_);

  Scheduler::RegisterIsolate(isolate_, uv_loop_);

  auxiliary_ = new uv_async_t;
  uv_async_init(uv_loop_, auxiliary_, [](uv_async_t* handle) {
    UVSchedulerSel* scheduler = static_cast<UVSchedulerSel*>(handle->data);
    if (!scheduler->running.load()) {
      uv_stop(handle->loop);
      return;
    }

    RunInspectorTasks(scheduler);
  });
  auxiliary_->data = this;
}
UVSchedulerSel::~UVSchedulerSel() {
  running.store(false);
  uv_async_send(auxiliary_);
  if (thread_.joinable()) {
    thread_.join();
  }
}
void UVSchedulerSel::RunTaskLoop() {
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

    node::FreeIsolateData(isolate_data_);
    PlatformDelegate::UnregisterIsolate(isolate_);
    isolate_->Dispose();

    uv_close(reinterpret_cast<uv_handle_t*>(auxiliary_),
             [](uv_handle_t* handle) {
               delete reinterpret_cast<uv_async_t*>(handle);
             });
    uv_loop_close(uv_loop_);
    delete uv_loop_;
  });
}

void UVSchedulerSel::RunInspectorTasks(UVSchedulerSel* scheduler) {
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
  uv_async_send(auxiliary_);
}

UVSchedulerPar::UVSchedulerPar(v8::Isolate* isolate) : Scheduler(isolate) {
  uv_loop_ = Scheduler::GetIsolateUvLoop(isolate_);
  auxiliary_ = new uv_async_t;
  uv_async_init(uv_loop_, auxiliary_, [](uv_async_t*) {});
  uv_unref(reinterpret_cast<uv_handle_t*>(auxiliary_));
}
UVSchedulerPar::~UVSchedulerPar() {
  uv_close(reinterpret_cast<uv_handle_t*>(auxiliary_), [](uv_handle_t* handle) {
    delete reinterpret_cast<uv_async_t*>(handle);
  });
}

}  // namespace svm
