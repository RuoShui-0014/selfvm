#pragma once

#include <node.h>
#include <uv.h>
#include <v8-platform.h>

#include <mutex>
#include <queue>

#include "native/timer_manager.h"

namespace svm {

class TimerManager;
class InspectorAgent;

class Scheduler {
 public:
  enum class TaskType { kMacro, kMicro, kInterrupt };
  using TaskQueue = std::queue<std::unique_ptr<v8::Task>>;

  virtual ~Scheduler() = default;

  static void RegisterIsolateScheduler(v8::Isolate* isolate,
                                       Scheduler* scheduler);
  static void UnregisterIsolateScheduler(v8::Isolate* isolate);
  static Scheduler* GetIsolateScheduler(v8::Isolate* isolate);

  virtual uv_loop_t* GetLoop() { return nullptr; }
  virtual std::shared_ptr<v8::TaskRunner> TaskRunner() { return {}; }
  virtual TimerManager* GetTimerManager() { return nullptr; }

  virtual void PostTask(std::unique_ptr<v8::Task> task, TaskType type) = 0;
  virtual uint32_t PostDelayedTask(std::unique_ptr<v8::Task> task,
                                   uint64_t ms,
                                   Timer::Type type) = 0;

  virtual void Ref() {}
  virtual void Unref() {}

  static Scheduler* node_scheduler;

  enum class Type { kNode, kSelf };
};

template <Scheduler::Type t>
class UVScheduler;

}  // namespace svm
