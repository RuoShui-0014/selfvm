//
// Created by ruoshui on 25-7-2.
//

#pragma once

#include "../isolate/script_wrappable.h"
#include "../isolate/task.h"
#include "../isolate/wrapper_type_info.h"

namespace svm {

class IsolateHandle;
class AsyncInfo;

using ContextId = v8::Context*;

class ContextHandle : public ScriptWrappable {
 public:
  static ContextHandle* Create(IsolateHandle* isolate_handle);

  explicit ContextHandle(IsolateHandle* isolate_handle, ContextId const address);
  ~ContextHandle() override;

  /*********************** js interface *************************/
  std::pair<uint8_t*, size_t> Eval(std::string script, std::string filename);
  void EvalAsync(std::unique_ptr<AsyncInfo> info,
                 std::string script,
                 std::string filename);
  void Release();

  IsolateHandle* GetIsolateHandle() const { return isolate_handle_.Get(); };
  ContextId GetContextId() const { return address_; }
  v8::Local<v8::Context> GetContext();

  v8::Isolate* GetIsolateSel();
  v8::Isolate* GetIsolatePar();

  void PostTaskToSel(std::unique_ptr<v8::Task> task);
  void PostTaskToPar(std::unique_ptr<v8::Task> task);

  void Trace(cppgc::Visitor* visitor) const override;

 private:
  cppgc::Member<IsolateHandle> isolate_handle_;
  ContextId const address_;
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
  friend V8ContextHandle;
  static const WrapperTypeInfo wrapper_type_info_;
};

}  // namespace svm
