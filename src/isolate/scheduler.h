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
  Scheduler() = default;
  virtual ~Scheduler() = default;

  static void RegisterIsolate(v8::Isolate* isolate, uv_loop_t* loop);
  static uv_loop_t* GetIsolateUvLoop(v8::Isolate* isolate);

  virtual void RunLoop() {}
  virtual uv_loop_t* GetUvLoop() const { return nullptr; }
  virtual std::shared_ptr<v8::TaskRunner> TaskRunner() const { return {}; }
  virtual void PostInspectorTask(std::unique_ptr<v8::Task> task) {}
  virtual void KeepAlive() {}
  virtual void WillDie() {}
};

class UVSchedulerSel : public Scheduler {
 public:
  explicit UVSchedulerSel(
      v8::Isolate* isolate,
      std::unique_ptr<node::ArrayBufferAllocator> allocator);
  ~UVSchedulerSel() override;

  void RunLoop() override;
  void KeepAlive() override;
  void WillDie() override;
  std::shared_ptr<v8::TaskRunner> TaskRunner() const override;
  uv_loop_t* GetUvLoop() const override { return uv_loop_; }
  void Stop() { uv_async_send(keep_alive_); }

  static void InspectorLoop(UVSchedulerSel* scheduler);
  void PostInspectorTask(std::unique_ptr<v8::Task> task) override;

 private:
  std::mutex mutex_;
  std::condition_variable cv_;
  std::queue<std::unique_ptr<v8::Task>> tasks_;

  v8::Isolate* isolate_;
  node::IsolateData* isolate_data_;
  std::unique_ptr<node::ArrayBufferAllocator> allocator_;

  uv_loop_t* uv_loop_;
  uv_async_t* keep_alive_;
  std::atomic<int> uv_ref_count{0};

  std::thread thread_;
  std::atomic<bool> running{false};
};

class UVSchedulerPar : public Scheduler {
 public:
  explicit UVSchedulerPar(v8::Isolate* isolate);
  ~UVSchedulerPar() override;

  void KeepAlive() override;
  void WillDie() override;
  std::shared_ptr<v8::TaskRunner> TaskRunner() const override;
  uv_loop_t* GetUvLoop() const override { return uv_loop_; }
  void Stop() { uv_async_send(keep_alive_); }

 private:
  v8::Isolate* isolate_;
  uv_loop_t* uv_loop_;
  uv_async_t* keep_alive_;
  std::atomic<int> uv_ref_count{0};
};

}  // namespace svm
