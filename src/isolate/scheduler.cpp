#include "scheduler.h"

#include <uv.h>

#include <map>

#include "base/logger.h"
#include "isolate/inspector_agent.h"
#include "isolate/platform_delegate.h"
#include "isolate_holder.h"
#include "native/timer_manager.h"

namespace svm {

namespace {
std::mutex mutex;
std::map<v8::Isolate*, Scheduler*> g_isolate_scheduler_map;
}  // namespace

void Scheduler::RegisterIsolateScheduler(v8::Isolate* isolate,
                                         Scheduler* scheduler) {
  std::lock_guard lock{mutex};
  g_isolate_scheduler_map.emplace(isolate, scheduler);
}
void Scheduler::UnregisterIsolateScheduler(v8::Isolate* isolate) {
  std::lock_guard lock{mutex};
  g_isolate_scheduler_map.erase(isolate);
}
Scheduler* Scheduler::GetIsolateScheduler(v8::Isolate* isolate) {
  if (const auto it{g_isolate_scheduler_map.find(isolate)};
      it != g_isolate_scheduler_map.end()) {
    return it->second;
  }
  return nullptr;
}

Scheduler* Scheduler::node_scheduler{nullptr};

}  // namespace svm
