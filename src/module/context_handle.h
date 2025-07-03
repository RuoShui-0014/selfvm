//
// Created by ruoshui on 25-7-2.
//

#pragma once

#include "../utils/script_wrappable.h"

namespace svm {

class IsolateHandle;

class ContextHandle : public ScriptWrappable {
 public:
  static v8::Local<v8::FunctionTemplate> Interface(v8::Isolate* isolate);
  static ContextHandle* Create(IsolateHandle* isolate_handle);

  explicit ContextHandle(IsolateHandle* isolate_handle,
                         v8::Local<v8::Context> context);
  ~ContextHandle() override;

  void Trace(cppgc::Visitor* visitor) const override;

 private:
  v8::Global<v8::Context> context_;

  cppgc::Member<IsolateHandle> isolate_handle_;
};

}  // namespace svm
