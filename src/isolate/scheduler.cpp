#include "scheduler.h"

#include "../utils/utils.h"

namespace svm {

ScriptTask::ScriptTask(v8::Isolate* isolate,
                       v8::Local<v8::Context> context,
                       std::string& script)
    : isolate_(isolate),
      context_(isolate, context),
      script_(std::move(script)) {}
ScriptTask::~ScriptTask() {
  context_.Reset();
}
void ScriptTask::Run() {

  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);
  v8::Local<v8::Context> context = context_.Get(isolate_);
  v8::Context::Scope scope(context);

  v8::TryCatch try_catch(isolate_);
  v8::Local<v8::Script> script;
  v8::Local<v8::String> code = toString(script_);
  v8::ScriptOrigin scriptOrigin = v8::ScriptOrigin(toString("vm_01"));
  if (v8::Script::Compile(context_.Get(isolate_), code, &scriptOrigin)
          .ToLocal(&script)) {
    v8::MaybeLocal<v8::Value> maybe_result = script->Run(context);
    if (!maybe_result.IsEmpty()) {
      result.Reset(isolate_, maybe_result.ToLocalChecked());
      return;
    }
  }

  if (try_catch.HasCaught()) {
    try_catch.ReThrow();
  }
  result.Reset(isolate_, Undefined(isolate_));
}

Scheduler::Scheduler() {
  // thread_ = std::thread(&Scheduler::Entry, this);
}
Scheduler::~Scheduler() {}

void Scheduler::PostTask(std::unique_ptr<v8::Task> task) {
  {
    std::lock_guard lock(queue_mutex_);
    tasks_.push(std::move(task));
  }
  cv_.notify_one();
}

void Scheduler::PostDelayedTask(std::unique_ptr<v8::Task> task,
                                double delay_in_seconds) {
  {
    std::lock_guard lock(queue_mutex_);
    tasks_.push(std::move(task));
  }
  cv_.notify_one();
}
void Scheduler::PostNonNestableTask(std::unique_ptr<v8::Task> task) {
  {
    std::lock_guard lock(queue_mutex_);
    tasks_.push(std::move(task));
  }
  cv_.notify_one();
}

template <class Type>
auto ExchangeDefault(Type& container) {
  return std::exchange(container, Type{});
}
void Scheduler::RunTask() {
  while (true) {
    TaskQueue tasks;
    TaskQueue handle_tasks;
    TaskQueue interrupts;
    {
      // Grab current tasks
      auto lock = Lock();
      tasks = ExchangeDefault(lock->tasks_);
      handle_tasks = ExchangeDefault(lock->handle_tasks_);
      interrupts = ExchangeDefault(lock->interrupts_);
      if (tasks.empty() && handle_tasks.empty() && interrupts.empty()) {
        // lock->DoneRunning();
        return;
      }
    }

    // Execute interrupt tasks
    while (!interrupts.empty()) {
      interrupts.front()->Run();
      interrupts.pop();
    }

    // Execute handle tasks
    while (!handle_tasks.empty()) {
      handle_tasks.front()->Run();
      handle_tasks.pop();
    }

    // Execute tasks
    while (!tasks.empty()) {
      tasks.front()->Run();
      tasks.pop();
    }
  }
}

[[noreturn]] void Scheduler::Entry() {
  while (true) {
    {
      std::unique_lock lock(exec_mutex_);
      cv_.wait_for(lock, std::chrono::milliseconds(10),
                   [&] { return !tasks_.empty(); });
    }
    RunTask();
  }
}

}  // namespace svm
