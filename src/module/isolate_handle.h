//
// Created by ruoshui on 25-7-2.
//

#pragma once

#include "../isolate/script_wrappable.h"
#include "../isolate/wrapper_type_info.h"

namespace svm {

class IsolateHolder;
class Scheduler;
class ContextHandle;
class ScriptHandle;
class SessionHandle;
struct IsolateParams;

class IsolateHandle : public ScriptWrappable {
 public:
  using ContextId = v8::Context*;
  using ScriptId = v8::UnboundScript*;

  static IsolateHandle* Create(IsolateParams& params);

  explicit IsolateHandle(IsolateParams& params);
  ~IsolateHandle() override;

  IsolateHolder* GetIsolateHolder() const { return isolate_holder_.get(); }
  v8::Isolate* GetIsolateSel() const;
  v8::Isolate* GetIsolatePar() const;

  Scheduler* GetSchedulerSel() const;
  Scheduler* GetSchedulerPar() const;

  v8::Local<v8::Context> GetContext(ContextId address) const;
  v8::Local<v8::UnboundScript> GetScript(ScriptId address) const;

  void PostTaskToSel(std::unique_ptr<v8::Task> task) const;
  void PostTaskToPar(std::unique_ptr<v8::Task> task) const;
  void PostInspectorTask(std::unique_ptr<v8::Task> task) const;

  void AddDebugContext(ContextHandle* context) const;

  /*********************** js interface *************************/
  ContextHandle* GetContextHandle();
  ContextHandle* CreateContext();
  void CreateContextAsync(v8::Isolate* isolate,
                          v8::Local<v8::Context> context,
                          v8::Local<v8::Promise::Resolver> resolver);
  ScriptHandle* CreateScript(std::string script, std::string filename);
  SessionHandle* GetInspectorSession();
  void IsolateGc();
  void Release();
  v8::HeapStatistics GetHeapStatistics() const;

  void Trace(cppgc::Visitor* visitor) const override;

 private:
  cppgc::Member<ContextHandle> default_context_;
  cppgc::Member<SessionHandle> session_handle_;
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
