#include "local_dom_window.h"

#include "../module/isolate_handle.h"
#include "../utils/utils.h"

namespace svm {

void WindowConstructCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  info.GetIsolate()->ThrowException(v8::Exception::TypeError(
      toString(info.GetIsolate(), "Illegal constructor")));
}

void IsolateExposedConstructCallback(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  info.GetReturnValue().Set(GetExposedInterfaceObject(
      info.GetIsolate(), info.Holder(), V8IsolateHandle::GetWrapperTypeInfo()));
}

LocalDOMWindow::LocalDOMWindow() = default;
LocalDOMWindow::~LocalDOMWindow() = default;

////////////////////////////////////////////////////////////////////////////////////////
void V8Window::InstallInterfaceTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::Template> interface_template) {
  ConstructItem constructor{"Window", 0, WindowConstructCallback};
  // AttributeItem attrs[]{
  //     {"context", ContextAttributeGetCallback, nullptr,
  //      v8::PropertyAttribute::ReadOnly, Dependence::kPrototype},
  // };
  // OperationItem operas[]{
  //     {"gc", 0, GcOperationCallback, v8::PropertyAttribute::DontDelete,
  //      Dependence::kPrototype},
  //     {"getHeapStatistics", 0, GetHeapStatisticsOperationCallback,
  //      v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
  //     {"release", 0, ReleaseOperationCallback,
  //      v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
  // };
  ExposedConstructItem exposedConstructs[]{
      {"Isolate", IsolateExposedConstructCallback, Dependence::kInstance},
  };

  InstallConstructor(isolate, interface_template, constructor);

  // v8::Local<v8::Signature> signature =
  //     v8::Local<v8::Signature>::Cast(interface_template);
  // InstallAttributes(isolate, interface_template, attrs, signature);
  // InstallOperations(isolate, interface_template, operas, signature);
  InstallExposedConstructs(isolate, interface_template, exposedConstructs);
}

const WrapperTypeInfo V8Window::wrapper_type_info_{
    "Window", V8Window::InstallInterfaceTemplate, nullptr};

}  // namespace svm
