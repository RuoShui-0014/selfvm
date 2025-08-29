#include "wrapper_type_info.h"

#include "per_isolate_data.h"

namespace svm {

v8::Local<v8::Template> WrapperTypeInfo::GetV8ClassTemplate(
    v8::Isolate* isolate) const {
  PerIsolateData* per_isolate_data{PerIsolateData::From(isolate)};
  v8::Local v8_template{per_isolate_data->FindV8Template(this)};
  if (!v8_template.IsEmpty()) {
    return v8_template;
  }

  v8_template = v8::FunctionTemplate::New(isolate);
  install_interface_template_func(isolate, v8_template);
  per_isolate_data->AddV8Template(this, v8_template);
  return v8_template;
}

}  // namespace svm
