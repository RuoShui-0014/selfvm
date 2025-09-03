#pragma once

#include "isolate/script_wrappable.h"
#include "isolate/wrapper_type_info.h"
#include "isolate/external_data.h"

namespace svm {

class IsolateHolder;
class IsolateHandle;
class ContextHandle;
class AsyncInfo;

using ContextId = v8::Context*;
using ScriptId = v8::UnboundScript*;

class ScriptHandle final : public ScriptWrappable {
 public:
  explicit ScriptHandle(IsolateHandle* isolate_handle, ScriptId address);
  ~ScriptHandle() override;

  std::shared_ptr<IsolateHolder> GetIsolateHolder() const;
  v8::Local<v8::UnboundScript> GetScript() const;

  /*********************** js interface *************************/
  CopyData Run(const ContextHandle* context_handle) const;
  void RunAsync(std::unique_ptr<AsyncInfo> info,
                ContextId context_id) const;
  void RunIgnored(const ContextHandle* context_handle) const;
  void Release() const;

  void Trace(cppgc::Visitor* visitor) const override;

 private:
  cppgc::Member<IsolateHandle> isolate_handle_;
  std::weak_ptr<IsolateHolder> isolate_holder_;
  ScriptId const script_id_;
};

class V8ScriptHandle {
 public:
  static constexpr const WrapperTypeInfo* GetWrapperTypeInfo() {
    return &wrapper_type_info_;
  }

  static void InstallInterfaceTemplate(
      v8::Isolate* isolate,
      v8::Local<v8::Template> interface_template);

 private:
  friend ScriptHandle;
  static const WrapperTypeInfo wrapper_type_info_;
};

}  // namespace svm
