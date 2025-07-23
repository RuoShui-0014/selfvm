//
// Created by ruoshui on 25-7-5.
//

#pragma once

#include <v8.h>

#include <future>

#include "../module/context_handle.h"
#include "../module/isolate_handle.h"
#include "../utils/utils.h"

namespace svm {

class ContextHandle;
class IsolateHandle;

// 同步任务
template <typename T>
class SyncTask : public v8::Task {
 public:
  explicit SyncTask() = default;
  ~SyncTask() override = default;

  std::future<T> GetFuture() { return promise_.get_future(); }
  void SetResult(const T& _Val) { promise_.set_value(_Val); }

 protected:
  std::promise<T> promise_;
};

template <typename T>
  requires std::is_void_v<T>
class SyncTask<T> : public v8::Task {
 public:
  explicit SyncTask() = default;
  ~SyncTask() override = default;
};

// 异步任务
class AsyncInfo {
 public:
  AsyncInfo(IsolateHandle* isolate_handle,
            v8::Isolate* isolate,
            RemoteHandle<v8::Context> context,
            RemoteHandle<v8::Promise::Resolver> resolver);
  ~AsyncInfo();

  IsolateHandle* isolate_handle;
  v8::Isolate* isolate;
  RemoteHandle<v8::Context> context;
  RemoteHandle<v8::Promise::Resolver> resolver;
};
class AsyncTask : public v8::Task {
 public:
  explicit AsyncTask(std::unique_ptr<AsyncInfo> info)
      : info_(std::move(info)) {}
  ~AsyncTask() override = default;

 protected:
  std::unique_ptr<AsyncInfo> info_;
};

}  // namespace svm
