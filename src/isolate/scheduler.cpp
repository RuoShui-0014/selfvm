#include "scheduler.h"

#include <iostream>
#include <map>

#include "../utils/utils.h"
#include "platform_delegate.h"
#include "uv.h"

namespace svm {
namespace {
std::mutex mutex;
std::map<v8::Isolate*, uv_loop_t*> isolate_map;
}  // namespace

void Scheduler::RegisterIsolate(v8::Isolate* isolate, uv_loop_t* loop) {
  std::lock_guard lock(mutex);
  isolate_map.emplace(isolate, loop);
}
uv_loop_t* Scheduler::GetIsolateUvLoop(v8::Isolate* isolate) {
  auto it = isolate_map.find(isolate);
  if (it != isolate_map.end()) {
    return it->second;
  }
  return nullptr;
}

IsolateScheduler::IsolateScheduler() = default;
IsolateScheduler::~IsolateScheduler() = default;

void IsolateScheduler::PostTask(std::unique_ptr<v8::Task> task) {}

void IsolateScheduler::PostDelayedTask(std::unique_ptr<v8::Task> task,
                                       double delay_in_seconds) {}

void IsolateScheduler::PostNonNestableTask(std::unique_ptr<v8::Task> task) {}

}  // namespace svm
