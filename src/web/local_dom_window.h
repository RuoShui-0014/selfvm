#pragma once

#include "base/raw_ptr.h"
#include "isolate/scheduler.h"
#include "isolate/script_wrappable.h"
#include "isolate/wrapper_type_info.h"

namespace svm {

class IsolateHolder;

class LocalDOMWindow final : public ScriptWrappable {
 public:
  explicit LocalDOMWindow(IsolateHolder* isolate_holder);
  ~LocalDOMWindow() override;

  IsolateHolder* GetIsolateHolder() const { return isolate_holder_.get(); }

  void PostTaskToSel(std::unique_ptr<v8::Task> task,
                     Scheduler::TaskType type) const;
  uint32_t PostTimeoutTaskToSel(std::unique_ptr<v8::Task> task,
                                uint64_t ms) const;
  uint32_t PostIntervalTaskToSel(std::unique_ptr<v8::Task> task,
                                 uint64_t ms) const;

  void PostTaskToPar(std::unique_ptr<v8::Task> task,
                     Scheduler::TaskType type) const;
  void PostDelayTaskToPar(std::unique_ptr<v8::Task> task, double delay) const;

  void ClearTimeout(uint32_t id) const;
  void ClearInterval(uint32_t id) const;

  LocalDOMWindow* window() { return this; }

  bool log{false};
  bool glog{false};

  static void LogGetCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
    v8::Isolate* isolate{info.GetIsolate()};

    const LocalDOMWindow* node_global{
        Unwrap<LocalDOMWindow>(isolate->GetCurrentContext()->Global())};
    info.GetReturnValue().Set(v8::Boolean::New(isolate, node_global->log));
  }
  static void LogSetCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
    v8::Isolate* isolate{info.GetIsolate()};

    LocalDOMWindow* node_global{
        Unwrap<LocalDOMWindow>(isolate->GetCurrentContext()->Global())};
    node_global->log = info[0]->IsTrue();
  }
  static void GlogGetCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
    v8::Isolate* isolate{info.GetIsolate()};

    const LocalDOMWindow* node_global{
        Unwrap<LocalDOMWindow>(isolate->GetCurrentContext()->Global())};
    info.GetReturnValue().Set(v8::Boolean::New(isolate, node_global->glog));
  }
  static void GlogSetCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
    v8::Isolate* isolate{info.GetIsolate()};

    LocalDOMWindow* node_global{
        Unwrap<LocalDOMWindow>(isolate->GetCurrentContext()->Global())};
    node_global->glog = info[0]->IsTrue();
  }

 private:
  base::raw_ptr<IsolateHolder> isolate_holder_;
};

class V8Window {
 public:
  static constexpr const WrapperTypeInfo* GetWrapperTypeInfo() {
    return &wrapper_type_info_;
  }

  static void InstallInterfaceTemplate(
      v8::Isolate* isolate,
      v8::Local<v8::Template> interface_template);

 private:
  friend LocalDOMWindow;
  static const WrapperTypeInfo wrapper_type_info_;
};

}  // namespace svm
