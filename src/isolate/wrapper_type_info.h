//
// Created by ruoshui on 25-7-4.
//

#pragma once

#include <v8.h>

namespace svm {

using InstallInterfaceTemplateFuncType =
    void (*)(v8::Isolate* isolate, v8::Local<v8::Template> interface_template);
enum IsolateData: uint32_t { kPerIsolateData = 0, };

struct WrapperTypeInfo final {
  const char* interface_name;
  InstallInterfaceTemplateFuncType install_interface_template_func;
  const WrapperTypeInfo* parent_class;

  v8::Local<v8::Template> GetV8ClassTemplate(v8::Isolate* isolate) const;
};

} // svm
