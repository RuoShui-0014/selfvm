#include "context_handle.h"

#include <iostream>

#include "../isolate/per_isolate_data.h"
#include "../utils/utils.h"
#include "isolate_handle.h"

namespace svm {

ContextHandle* ContextHandle::Create(IsolateHandle* isolate_handle) {
  v8::Local<v8::Context> context =
      isolate_handle->GetIsolateHolder()->NewContext();
  v8::Isolate* isolate = isolate_handle->GetParentIsolate();
  ContextHandle* context_handle =
      MakeCppGcObject<GC::kSpecified, ContextHandle>(isolate, isolate_handle,
                                                     context);
  v8::Local<v8::FunctionTemplate> isolate_handle_template =
      V8ContextHandle::GetWrapperTypeInfo()
          ->GetV8ClassTemplate(isolate)
          .As<v8::FunctionTemplate>();
  v8::Local<v8::Object> obj = isolate_handle_template->InstanceTemplate()
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

///////////////////////////////////////////////////////////////////////////////
void V8ContextHandle::InstallInterfaceTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::Template> interface_template) {
  ConstructItem constructor{"Context", 0, nullptr};
  // AttributeItem attrs[]{
  //     {"context", AttributeGetterContextCallback, nullptr,
  //      v8::PropertyAttribute::ReadOnly, Dependence::kPrototype},
  // };
  // OperationItem operas[]{};

  InstallConstructor(isolate, interface_template, constructor);

  // v8::Local<v8::Signature> signature =
  // v8::Local<v8::Signature>::Cast(interface_template);
  // InstallAttributes(isolate, interface_template, attrs, signature);
  // InstallOperations(isolate, interface, operas, signature);
}

const WrapperTypeInfo V8ContextHandle::wrapper_type_info_{
    "Isolate", V8ContextHandle::InstallInterfaceTemplate, nullptr};

}  // namespace svm
