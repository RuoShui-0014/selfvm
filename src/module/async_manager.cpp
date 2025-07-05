#include "async_manager.h"

#include <node.h>
#include <uv.h>

#include <iostream>

namespace svm {

AsyncManager::AsyncManager()
    : uv_loop_{node::GetCurrentEventLoop(v8::Isolate::GetCurrent())},
      uv_async_{new uv_async_t} {
  uv_async_init(uv_loop_, uv_async_, [](uv_async_t* uv_async) {
    auto& async_manager = *static_cast<AsyncManager*>(uv_async->data);
    TaskQueue tasks;
    {
      std::lock_guard lock(async_manager.mutex_);
      tasks = std::exchange(async_manager.tasks_, {});
      if (tasks.empty()) {
        return;
      }
    }
    while (!tasks.empty()) {
      tasks.front()->Run();
      tasks.pop();
    }
  });

  uv_async_->data = this;
  uv_unref(reinterpret_cast<uv_handle_t*>(uv_async_));
}

AsyncManager::~AsyncManager() {
  uv_close(reinterpret_cast<uv_handle_t*>(uv_async_), [](uv_handle_t* handle) {
    delete reinterpret_cast<uv_async_t*>(handle);
  });
}

void AsyncManager::PostTask(std::unique_ptr<v8::Task> task) {
  std::lock_guard lock(mutex_);
  tasks_.emplace(std::move(task));
}

void AsyncManager::Send() const {
  uv_async_send(uv_async_);
}

}  // namespace svm
