#include "timer_manager.h"

#include <ranges>

#include "base/check.h"
#include "isolate/isolate_holder.h"

namespace svm {

TimerManager::TimerManager(uv_loop_t* loop) : loop_{loop} {}
TimerManager::~TimerManager() = default;

void TimerManager::OnCallback(uv_timer_t* time_t) {
  v8::Isolate* isolate{v8::Isolate::GetCurrent()};
  v8::HandleScope handle_scope{isolate};
  v8::MicrotasksScope scope{isolate, v8::MicrotasksScope::kRunMicrotasks};

  if (const Timer * timer{static_cast<Timer*>(time_t->data)};
      timer->type == Timer::Type::ktimeout) {
    timer->task->Run();
    timer->timer_manager->StopTimer(timer->id);
  } else {
    timer->task->Run();
    uv_timer_again(timer->timer.get());
  }
}

uint32_t TimerManager::AddTimer(std::unique_ptr<v8::Task> task,
                                uint64_t ms,
                                Timer::Type type) {
  uint32_t id{next_timer_id_++};
  auto timer{std::make_unique<Timer>(
      this, std::move(task), std::make_unique<uv_timer_t>(), ms, id, type)};
  timer->timer->data = timer.get();
  uv_timer_init(loop_, timer->timer.get());
  uv_timer_start(timer->timer.get(), OnCallback, ms, 0);
  timers_.emplace(id, std::move(timer));
  return id;
}

uint32_t TimerManager::AddTimer(std::unique_ptr<Timer> timer) {
  uint32_t id{next_timer_id_++};
  timers_.emplace(id, std::move(timer));
  return id;
}

void TimerManager::StopTimer(const uint32_t id) {
  if (const auto it{timers_.find(id)}; it != timers_.end()) {
    timers_.erase(it);
  }
}
void TimerManager::ClearAllTimer() {
  for (auto& val : timers_ | std::views::values) {
    val.reset();
  }
}

}  // namespace svm
