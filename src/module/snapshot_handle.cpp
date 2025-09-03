#include "snapshot_handle.h"

namespace svm {

SnapshotHandle::SnapshotHandle() {
  v8::SnapshotCreator creator{v8::Isolate::GetCurrent()};
}
SnapshotHandle::~SnapshotHandle() = default;

void SnapshotHandle::SetContext(ContextHandle* context_handle) {

}

void SnapshotHandle::Trace(cppgc::Visitor* visitor) const {
  ScriptWrappable::Trace(visitor);
}

void V8SnapshotHandle::InstallInterfaceTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::Template> interface_template) {
  ConstructConfig constructor{"Snapshot", 0, nullptr};
  InstallConstructor(isolate, interface_template, constructor);

  //   OperationConfig operas[]{
  //     {"connect", 1, ConnectOperationCallback,
  //      v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
  // };
  //
  //   v8::Local signature{v8::Local<v8::Signature>::Cast(interface_template)};
  //   InstallOperations(isolate, interface_template, signature, operas);
}

const WrapperTypeInfo V8SnapshotHandle::wrapper_type_info_{
    "Snapshot", V8SnapshotHandle::InstallInterfaceTemplate, nullptr};

}  // namespace svm
