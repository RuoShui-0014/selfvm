#pragma once

#include <memory>

#include "isolate/task.h"
#include "uv.h"

namespace svm {

class TimerManager;

struct Timer {
  enum class Type { ktimeout, kInterval };
  ~Timer() { uv_timer_stop(timer.get()); }

  TimerManager* timer_manager;
  std::unique_ptr<v8::Task> task;
  std::unique_ptr<uv_timer_t> timer;
  uint32_t delay;
  uint32_t id;
  Type type;
};

class TimerManager {
 public:
  explicit TimerManager(uv_loop_t* loop);
  ~TimerManager();

  static void OnCallback(uv_timer_t* time_t);

  uint32_t AddTimer(std::unique_ptr<v8::Task> task,
                    uint64_t ms,
                    Timer::Type type);
  uint32_t AddTimer(std::unique_ptr<Timer> timer);
  void StopTimer(uint32_t id);
  void ClearAllTimer();

 private:
  uv_loop_t* loop_{nullptr};
  uint32_t next_timer_id_{1};
  std::unordered_map<uint32_t, std::unique_ptr<Timer>> timers_;
};

}  // namespace svm
