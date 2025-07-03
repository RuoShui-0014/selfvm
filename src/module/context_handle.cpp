#include "context_handle.h"

#include <iostream>

#include "../utils/utils.h"
#include "isolate_handle.h"

namespace svm {

v8::Local<v8::FunctionTemplate> ContextHandle::Interface(v8::Isolate* isolate) {
  ConstructItem constructor{"Context", 0, nullptr};
  // AttributeItem attrs[]{
  //     {"context", AttributeGetterContextCallback, nullptr,
  //      v8::PropertyAttribute::ReadOnly, Dependence::kPrototype},
  // };
  // OperationItem operas[]{};

  v8::Local<v8::FunctionTemplate> interface =
      InstallConstructor(isolate, constructor);

  return interface;
}

ContextHandle* ContextHandle::Create(IsolateHandle* isolate_handle) {
  v8::Local<v8::Context> context =
      isolate_handle->GetIsolateHolder()->NewContext();
  v8::Isolate* isolate = isolate_handle->GetIsolateHolder()->GetParentIsolate();
  ContextHandle* context_handle =
      MakeCppGcObject<ContextHandle>(isolate, isolate_handle, context);
  v8::Local<v8::Object> obj = ContextHandle::Interface(isolate)
                                  ->InstanceTemplate()
                                  ->NewInstance(isolate->GetCurrentContext())
                                  .ToLocalChecked();
  ScriptWrappable::Wrap(obj, context_handle);
  return context_handle;
}

ContextHandle::ContextHandle(IsolateHandle* isolate_handle,
                             v8::Local<v8::Context> context)
    : isolate_handle_(isolate_handle) {
  context_.Reset(isolate_handle->GetIsolate(), context);
  std::cout << "ContextHandle()" << std::endl;
}

ContextHandle::~ContextHandle() {
  context_.Clear();
  std::cout << "~ContextHandle()" << std::endl;
}

void ContextHandle::Trace(cppgc::Visitor* visitor) const {
  visitor->Trace(isolate_handle_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace svm
