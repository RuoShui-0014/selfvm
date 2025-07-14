//
// Created by ruoshui on 25-7-3.
//

#pragma once

#include <node.h>
#include <uv.h>
#include <v8-platform.h>

#include <mutex>
#include <queue>

namespace svm {

class Scheduler {
 public:
  explicit Scheduler(v8::Isolate* isolate) : isolate_(isolate) {}
  virtual ~Scheduler() {
    isolate_ = nullptr;
    uv_loop_ = nullptr;
    auxiliary_ = nullptr;
  }

  static void RegisterIsolate(v8::Isolate* isolate, uv_loop_t* loop);
  static uv_loop_t* GetIsolateUvLoop(const v8::Isolate* isolate);

  virtual uv_loop_t* GetUvLoop() const { return uv_loop_; }
  virtual std::shared_ptr<v8::TaskRunner> TaskRunner() const;
  virtual void PostInspectorTask(std::unique_ptr<v8::Task> task) {}
  virtual void KeepAlive() {
    if (++uv_ref_count == 1) {
      uv_ref(reinterpret_cast<uv_handle_t*>(auxiliary_));
    }
  }
  virtual void WillDie() {
    if (--uv_ref_count == 0) {
      uv_unref(reinterpret_cast<uv_handle_t*>(auxiliary_));
    }
  }

 protected:
  v8::Isolate* isolate_{nullptr};
  uv_loop_t* uv_loop_{nullptr};
  uv_async_t* auxiliary_{nullptr};
  std::atomic<int> uv_ref_count{0};
};

class UVSchedulerSel : public Scheduler {
 public:
  explicit UVSchedulerSel(
      v8::Isolate* isolate,
      std::unique_ptr<node::ArrayBufferAllocator> allocator);
  ~UVSchedulerSel() override;

  void RunTaskLoop();
  static void RunInspectorTasks(UVSchedulerSel* scheduler);
  void PostInspectorTask(std::unique_ptr<v8::Task> task) override;

 private:
  node::IsolateData* isolate_data_{nullptr};
  std::unique_ptr<node::ArrayBufferAllocator> allocator_;

  std::mutex mutex_;
  std::condition_variable cv_;
  std::queue<std::unique_ptr<v8::Task>> tasks_;

  std::thread thread_;
  std::atomic<bool> running{false};
};

class UVSchedulerPar : public Scheduler {
 public:
  explicit UVSchedulerPar(v8::Isolate* isolate);
  ~UVSchedulerPar() override;
};

}  // namespace svm
