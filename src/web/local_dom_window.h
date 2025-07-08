//
// Created by ruoshui on 25-7-8.
//

#pragma once

#include "../isolate/script_wrappable.h"
#include "../isolate/wrapper_type_info.h"

namespace svm {

class LocalDOMWindow : public ScriptWrappable {
 public:
  LocalDOMWindow();
  ~LocalDOMWindow() override;
};

class V8Window {
 public:
  static constexpr const WrapperTypeInfo* GetWrapperTypeInfo() {
    return &wrapper_type_info_;
  }

  static void InstallInterfaceTemplate(
      v8::Isolate* isolate,
      v8::Local<v8::Template> interface_template);

 private:
  friend LocalDOMWindow;
  static const WrapperTypeInfo wrapper_type_info_;
};

}  // namespace svm
