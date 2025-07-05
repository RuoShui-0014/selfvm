//
// Created by ruoshui on 25-7-2.
//

#pragma once

#include "../isolate/script_wrappable.h"
#include "../isolate/wrapper_type_info.h"

namespace svm {

class IsolateHandle;

class ContextHandle : public ScriptWrappable {
 public:
  static ContextHandle* Create(IsolateHandle* isolate_handle);

  explicit ContextHandle(IsolateHandle* isolate_handle,
                         v8::Local<v8::Context> context);
  ~ContextHandle() override;

  void EvalSync(std::string script, v8::Local<v8::Value>* result) const;
  void EvalAsync(std::string script, v8::Local<v8::Promise::Resolver> result) const;
  void Release();

  void Trace(cppgc::Visitor* visitor) const override;

 private:
  v8::Global<v8::Context> context_;

  cppgc::Member<IsolateHandle> isolate_handle_;
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
