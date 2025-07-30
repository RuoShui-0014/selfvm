//
// Created by ruoshui on 25-7-5.
//

#pragma once

#include <v8.h>

#include <future>

#include "../utils/utils.h"

namespace svm {

class ContextHandle;
class IsolateHandle;
class IsolateHolder;

template <typename T>
class SyncTask;

// 同步任务
template <typename T>
class future {
 public:
  T& get() {
    while (!finished_.load()) {
    }
    return value_;
  }
  void set_value(const T& value) {
    value_ = value;
    finished_.store(true);
  }
  void set_value(T&& value) {
    value_ = std::move(value);
    finished_.store(true);
  }

 private:
  std::atomic_bool finished_{false};
  T value_;
};
template <typename T>
class SyncFastTask : public v8::Task {
 public:
  explicit SyncFastTask() = default;
  ~SyncFastTask() override = default;

  std::unique_ptr<future<T>> GetFuture() {
    auto fut = std::make_unique<future<T>>();
    future_ = fut.get();
    return fut;
  }
  void SetResult(const T& _Val) {
    if (future_) {
      future_->set_value(_Val);
    }
  }

 protected:
  future<T>* future_{};
};

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
  AsyncInfo(std::shared_ptr<IsolateHolder> isolate_holder,
            RemoteHandle<v8::Context> context,
            RemoteHandle<v8::Promise::Resolver> resolver);
  ~AsyncInfo();

  v8::Isolate* GetIsolateSel() const;
  v8::Isolate* GetIsolatePar() const;
  void PostHandleTaskToPar(std::unique_ptr<v8::Task> task) const;

  std::shared_ptr<IsolateHolder> isolate_holder_;
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
