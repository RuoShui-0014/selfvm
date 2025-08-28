//
// Created by ruoshui on 25-7-5.
//

#pragma once

#include <v8.h>

#include <future>

#include "../base/waiter.h"
#include "../utils/utils.h"

namespace svm {

class IsolateHolder;

// 同步任务
template <typename T>
class SyncTask : public v8::Task {
 public:
  explicit SyncTask() = default;
  ~SyncTask() override = default;

  std::unique_ptr<base::Waiter<T>> CreateWaiter() {
    auto waiter{std::make_unique<base::Waiter<T>>()};
    waiter_ = waiter.get();
    return waiter;
  }

  base::Waiter<T>* GetWaiter() { return waiter_; }

  void SetWaiter(base::Waiter<T>* waiter) { waiter_ = waiter; }

  void SetResult(const T& _Val) {
    if (waiter_) {
      waiter_->SetValue(_Val);
    }
  }

 protected:
  base::Waiter<T>* waiter_{nullptr};
};

template <>
class SyncTask<void> : public v8::Task {
 public:
  explicit SyncTask() = default;
  ~SyncTask() override = default;
};

// 异步任务
class AsyncInfo {
 public:
  AsyncInfo(const std::shared_ptr<IsolateHolder>& isolate_holder,
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
  explicit AsyncTask(std::unique_ptr<AsyncInfo> info);
  ~AsyncTask() override;

 protected:
  std::unique_ptr<AsyncInfo> info_;
};

}  // namespace svm
