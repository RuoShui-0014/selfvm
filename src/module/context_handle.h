#pragma once

#include <cppgc/member.h>

#include "isolate/external_data.h"
#include "isolate/script_wrappable.h"
#include "isolate/task.h"
#include "isolate/wrapper_type_info.h"

namespace svm {

class IsolateHolder;
class IsolateHandle;
class AsyncInfo;

using ContextId = v8::Context*;
using ScriptId = v8::UnboundScript*;

class ContextHandle final : public ScriptWrappable {
 public:
  explicit ContextHandle(IsolateHandle* isolate_handle, ContextId address);
  ~ContextHandle() override;

  /*********************** js interface *************************/
  CopyData Eval(std::string script, std::string filename) const;
  void EvalIgnored(std::string script, std::string filename) const;
  void EvalAsync(std::unique_ptr<AsyncInfo> info,
                 std::string script,
                 std::string filename) const;
  void Release() const;

  IsolateHandle* GetIsolateHandle() const { return isolate_handle_.Get(); };
  std::shared_ptr<IsolateHolder> GetIsolateHolder() const;
  ContextId GetContextId() const { return context_id_; }
  v8::Local<v8::Context> GetContext() const;

  v8::Isolate* GetIsolateSel() const;
  v8::Isolate* GetIsolatePar() const;

  void Trace(cppgc::Visitor* visitor) const override;

 private:
  cppgc::Member<IsolateHandle> isolate_handle_;
  std::weak_ptr<IsolateHolder> isolate_holder_;
  const ContextId context_id_;
};

class V8ContextHandle {
 public:
  static constexpr const WrapperTypeInfo* GetWrapperTypeInfo() {
    return &wrapper_type_info_;
  }

  static void InstallInterfaceTemplate(
      v8::Isolate* isolate,
      v8::Local<v8::Template> interface_template);

 private:
  friend ContextHandle;
  static const WrapperTypeInfo wrapper_type_info_;
};

}  // namespace svm
