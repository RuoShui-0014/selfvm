//
// Created by ruoshui on 25-7-2.
//

#pragma once

#include "../isolate/isolate_holder.h"
#include "../isolate/script_wrappable.h"
#include "../isolate/wrapper_type_info.h"

namespace svm {

class ContextHandle;
class SessionHandle;

class IsolateHandle : public ScriptWrappable {
 public:
  static IsolateHandle* Create(v8::Isolate* isolate);

  explicit IsolateHandle(std::unique_ptr<IsolateHolder>);
  ~IsolateHandle() override;

  IsolateHolder* GetIsolateHolder() const { return isolate_holder_.get(); }
  v8::Isolate* GetIsolateSel() const {
    return isolate_holder_->GetIsolateSel();
  }
  v8::Isolate* GetIsolatePar() const {
    return isolate_holder_->GetIsolatePar();
  }
  ContextHandle* GetContextHandle();
  v8::Local<v8::Context> GetContext(uint32_t id);

  Scheduler* GetSchedulerSel();

  void PostTaskToSel(std::unique_ptr<v8::Task> task);
  void PostTaskToPar(std::unique_ptr<v8::Task> task);
  void PostInspectorTask(std::unique_ptr<v8::Task> task);

  /*********************** js interface *************************/
  ContextHandle* CreateContext();
  void CreateContextAsync(v8::Isolate* isolate,
                          v8::Local<v8::Context> context,
                          v8::Local<v8::Promise::Resolver> resolver);

  SessionHandle* CreateInspectorSession();
  void IsolateGc();
  void Release();
  v8::HeapStatistics GetHeapStatistics() const;

  void Trace(cppgc::Visitor* visitor) const override;

 private:
  cppgc::Member<ContextHandle> default_context_;
  std::unique_ptr<IsolateHolder> isolate_holder_;
};

class V8IsolateHandle {
 public:
  static constexpr const WrapperTypeInfo* GetWrapperTypeInfo() {
    return &wrapper_type_info_;
  }

  static void InstallInterfaceTemplate(
      v8::Isolate* isolate,
      v8::Local<v8::Template> interface_template);

 private:
  friend IsolateHandle;
  static const WrapperTypeInfo wrapper_type_info_;
};

}  // namespace svm
