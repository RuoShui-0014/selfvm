#pragma once

#include "isolate/script_wrappable.h"
#include "isolate/wrapper_type_info.h"

namespace svm {

class IsolateHolder;
class Scheduler;
class ContextHandle;
class ScriptHandle;
class SessionHandle;
struct IsolateParams;
class AsyncInfo;

class IsolateHandle final : public ScriptWrappable {
 public:
  using ContextId = v8::Context*;
  using ScriptId = v8::UnboundScript*;

  static IsolateHandle* Create(IsolateParams& params);

  explicit IsolateHandle(IsolateParams& params);
  ~IsolateHandle() override;

  std::shared_ptr<IsolateHolder> GetIsolateHolder() const;

  v8::Local<v8::Context> GetContext(ContextId address) const;
  v8::Local<v8::UnboundScript> GetScript(ScriptId address) const;

  /*********************** js interface *************************/
  ContextHandle* GetContextHandle();
  ContextHandle* CreateContext();
  void CreateContextAsync(std::unique_ptr<AsyncInfo> info);
  ScriptHandle* CreateScript(std::string script, std::string filename);
  void CreateScriptAsync(std::unique_ptr<AsyncInfo> info,
                         std::string script,
                         std::string filename);
  SessionHandle* GetInspectorSession();
  void IsolateGc() const;
  void Release();
  v8::HeapStatistics GetHeapStatistics() const;

  void Trace(cppgc::Visitor* visitor) const override;

 private:
  cppgc::Member<ContextHandle> context_handle_;
  cppgc::Member<SessionHandle> session_handle_;
  std::shared_ptr<IsolateHolder> isolate_holder_;
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
