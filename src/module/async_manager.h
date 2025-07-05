//
// Created by ruoshui on 25-7-6.
//

#pragma once

#include <uv.h>
#include <v8.h>
#include <mutex>
#include <queue>

namespace svm {

class AsyncManager {
  using TaskQueue = std::queue<std::unique_ptr<v8::Task>>;
 public:
  explicit AsyncManager();
  ~AsyncManager();

  void PostTask(std::unique_ptr<v8::Task> task);
  void Send() const;

 private:
  uv_loop_t* uv_loop_;
  uv_async_t* uv_async_;

  std::mutex mutex_;
  TaskQueue tasks_;
};

}  // namespace svm
