//
// Created by ruoshui on 25-7-2.
//

#pragma once

#include "../utils/script_wrappable.h"
#include "context_handle.h"

namespace svm {

class IsolateHandle : public ScriptWrappable {
 public:
  static v8::Local<v8::FunctionTemplate> Interface(v8::Isolate* isolate);
  static IsolateHandle* Create();

  IsolateHandle();
  ~IsolateHandle() override;

 private:
  cppgc::Member<ContextHandle> context_;
};



}  // namespace svm
