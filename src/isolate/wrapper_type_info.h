#pragma once

#include <v8.h>

#include "isolate/script_wrappable.h"

namespace svm {

using InstallInterfaceTemplateFuncType =
    void (*)(v8::Isolate* isolate, v8::Local<v8::Template> interface_template);
enum IsolateData : uint32_t {
  kPerIsolateData = 0,
};

struct WrapperTypeInfo final {
  const char* interface_name;
  InstallInterfaceTemplateFuncType install_interface_template_func;
  const WrapperTypeInfo* parent_class;

  v8::Local<v8::Template> GetV8ClassTemplate(v8::Isolate* isolate) const;
};

template <typename T>
v8::Local<v8::Function> NewFunction(v8::Isolate* isolate,
                                    v8::Local<v8::Context> context) {
  const v8::Local<v8::FunctionTemplate> handle_template{
      T::GetWrapperTypeInfo()
          ->GetV8ClassTemplate(isolate)
          .template As<v8::FunctionTemplate>()};
  return {handle_template->GetFunction(context).ToLocalChecked()};
}

template <typename T, typename Wrappable>
v8::Local<v8::Object> NewInstance(v8::Isolate* isolate,
                                  v8::Local<v8::Context> context,
                                  Wrappable* wrappable) {
  const v8::Local<v8::FunctionTemplate> handle_template{
      T::GetWrapperTypeInfo()
          ->GetV8ClassTemplate(isolate)
          .template As<v8::FunctionTemplate>()};
  v8::Local target{handle_template->InstanceTemplate()
                       ->NewInstance(context)
                       .ToLocalChecked()};
  ScriptWrappable::Wrap(target, wrappable);
  return target;
}

template <typename T>
bool IsInstance(v8::Isolate* isolate, v8::Local<v8::Value> target) {
  const v8::Local<v8::FunctionTemplate> handle_template{
      T::GetWrapperTypeInfo()
          ->GetV8ClassTemplate(isolate)
          .template As<v8::FunctionTemplate>()};
  return handle_template->HasInstance(target);
}

}  // namespace svm
