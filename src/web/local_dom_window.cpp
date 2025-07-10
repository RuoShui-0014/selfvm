#include "local_dom_window.h"

#include "../module/isolate_handle.h"
#include "../utils/utils.h"

namespace svm {

void WindowConstructCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  info.GetIsolate()->ThrowException(v8::Exception::TypeError(
      toString(info.GetIsolate(), "Illegal constructor")));
}

void WindowAttributeGetCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> receiver = info.This();
  LocalDOMWindow* blink_receiver =
      ScriptWrappable::Unwrap<LocalDOMWindow>(receiver);
  LocalDOMWindow* return_value = blink_receiver->window();
  V8SetReturnValue(info, return_value);
}

void AtobOperationCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> receiver = info.This();

  LocalDOMWindow* blink_receiver =
      ScriptWrappable::Unwrap<LocalDOMWindow>(receiver);
  LocalDOMWindow* return_value = blink_receiver->window();

  // info.GetReturnValue().Set();
}

void BtoaOperationCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  if (info.Length() < 1) {
    isolate->ThrowException(v8::Exception::TypeError(
        toString(info.GetIsolate(),
                 "Failed to execute 'btoa' on 'Window': 1 argument required, "
                 "but only 0 present.")));
    return;
  }

  char* str = *v8::String::Utf8Value(isolate, info[0]);
  info.GetReturnValue().Set(
      node::Encode(info.GetIsolate(), str, strlen(str), node::BASE64));
}

void IsolateExposedConstructCallback(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  info.GetReturnValue().Set(GetExposedInterfaceObject(
      info.GetIsolate(), info.Holder(), V8IsolateHandle::GetWrapperTypeInfo()));
}

void WindowExposedConstructCallback(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  info.GetReturnValue().Set(GetExposedInterfaceObject(
      info.GetIsolate(), info.Holder(), V8Window::GetWrapperTypeInfo()));
}

LocalDOMWindow::LocalDOMWindow() = default;
LocalDOMWindow::~LocalDOMWindow() {

}

////////////////////////////////////////////////////////////////////////////////////////
void V8Window::InstallInterfaceTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::Template> interface_template) {
  ConstructConfig constructor{"Window", 0, WindowConstructCallback};
  AttributeConfig attrs[]{
      {"window", WindowAttributeGetCallback, nullptr,
       v8::PropertyAttribute::ReadOnly, Dependence::kInstance},
  };
  OperationConfig operas[]{
      {"atob", 1, AtobOperationCallback, v8::PropertyAttribute::DontDelete,
       Dependence::kInstance},
      {"btoa", 1, BtoaOperationCallback, v8::PropertyAttribute::DontDelete,
       Dependence::kInstance},
  };
  ExposedConstructConfig exposedConstructs[]{
      {"Isolate", IsolateExposedConstructCallback, Dependence::kInstance},
      {"Window", WindowExposedConstructCallback, Dependence::kInstance},
  };

  InstallConstructor(isolate, interface_template, constructor);

  v8::Local<v8::Signature> signature =
      v8::Local<v8::Signature>::Cast(interface_template);
  InstallAttributes(isolate, interface_template, signature, attrs);
  InstallOperations(isolate, interface_template, signature, operas);
  InstallExposedConstructs(isolate, interface_template, exposedConstructs);
}

const WrapperTypeInfo V8Window::wrapper_type_info_{
    "Window", V8Window::InstallInterfaceTemplate, nullptr};

}  // namespace svm
