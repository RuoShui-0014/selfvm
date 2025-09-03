#pragma once

#include "context_handle.h"
#include "isolate/script_wrappable.h"
#include "isolate/wrapper_type_info.h"

namespace svm {

class ContextHandle;

class SnapshotHandle final : public ScriptWrappable {
 public:
  SnapshotHandle();
  ~SnapshotHandle() override;

  void SetContext(ContextHandle* context_handle);

  void Trace(cppgc::Visitor* visitor) const override;
};

class V8SnapshotHandle {
 public:
  static constexpr const WrapperTypeInfo* GetWrapperTypeInfo() {
    return &wrapper_type_info_;
  }

  static void InstallInterfaceTemplate(
      v8::Isolate* isolate,
      v8::Local<v8::Template> interface_template);

 private:
  friend SnapshotHandle;
  static const WrapperTypeInfo wrapper_type_info_;
};

}  // namespace svm
