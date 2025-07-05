//
// Created by ruoshui on 25-7-2.
//

#pragma once

#include "../isolate/isolate_holder.h"
#include "../isolate/script_wrappable.h"
#include "../isolate/wrapper_type_info.h"
#include "async_manager.h"

namespace svm {

class ContextHandle;

class IsolateHandle : public ScriptWrappable {
 public:
  static IsolateHandle* Create(v8::Isolate* isolate);

  explicit IsolateHandle(std::unique_ptr<IsolateHolder>&);
  ~IsolateHandle() override;

  IsolateHolder* GetIsolateHolder() const { return isolate_holder_.get(); }
  v8::Isolate* GetIsolate() const { return isolate_holder_->GetIsolate(); }
  v8::Isolate* GetParentIsolate() const {
    return isolate_holder_->GetParentIsolate();
  }
  AsyncManager* GetAsyncManager() const { return async_manager_.get(); }
  ContextHandle* GetContextHandle();

  void Release();

  void Trace(cppgc::Visitor* visitor) const override;

 private:
  std::unique_ptr<AsyncManager> async_manager_{
      std::make_unique<AsyncManager>()};
  std::unique_ptr<IsolateHolder> isolate_holder_;

  cppgc::Member<ContextHandle> default_context_;
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
