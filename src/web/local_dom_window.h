#pragma once

#include "../base/raw_ptr.h"
#include "../isolate/script_wrappable.h"
#include "../isolate/wrapper_type_info.h"

namespace svm {

class IsolateHolder;

class LocalDOMWindow final : public ScriptWrappable {
 public:
  explicit LocalDOMWindow(IsolateHolder* isolate_holder);
  ~LocalDOMWindow() override;

  IsolateHolder* GetIsolateHolder() const { return isolate_holder_.get(); }

  void PostMacroTaskToSel(std::unique_ptr<v8::Task> task) const;
  uint32_t PostTimeoutTaskToSel(std::unique_ptr<v8::Task> task,
                                uint64_t ms) const;
  uint32_t PostIntervalTaskToSel(std::unique_ptr<v8::Task> task,
                                 uint64_t ms) const;

  void PostMacroTaskToPar(std::unique_ptr<v8::Task> task) const;
  void PostDelayTaskToPar(std::unique_ptr<v8::Task> task, double delay) const;

  void ClearTimout(uint32_t id) const;
  void ClearInterval(uint32_t id) const;

  LocalDOMWindow* window() { return this; }

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
