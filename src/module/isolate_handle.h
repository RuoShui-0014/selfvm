//
// Created by ruoshui on 25-7-2.
//

#pragma once

#include "../isolate/isolate_holder.h"
#include "../utils/script_wrappable.h"

namespace svm {

class ContextHandle;

class IsolateHandle : public ScriptWrappable {
 public:
  static v8::Local<v8::FunctionTemplate> Interface(v8::Isolate* isolate);
  static IsolateHandle* Create(v8::Isolate* isolate);

  explicit IsolateHandle(std::unique_ptr<IsolateHolder>&);
  ~IsolateHandle() override;

  void SetRef(ContextHandle* other) {
    default_context_ = other;  // 通过cppgc::Member自动触发屏障
  }

  IsolateHolder* GetIsolateHolder() const { return isolate_holder_.get(); }
  v8::Isolate* GetIsolate() const { return isolate_holder_->GetIsolate(); }

  ContextHandle* GetContextHandle();

  void Trace(cppgc::Visitor* visitor) const override;

 private:
  std::unique_ptr<IsolateHolder> isolate_holder_;

  cppgc::Member<ContextHandle> default_context_;
};

}  // namespace svm
