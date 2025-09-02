#pragma once

#include <v8.h>

#include <span>

namespace svm {

inline v8::Local<v8::String> v8_str(const char* str) {
  return v8::String::NewFromOneByte(v8::Isolate::GetCurrent(),
                                    reinterpret_cast<const uint8_t*>(str),
                                    v8::NewStringType::kInternalized)
      .ToLocalChecked();
}
inline v8::Local<v8::String> v8_str(v8::Isolate* isolate, const char* str) {
  return v8::String::NewFromOneByte(isolate,
                                    reinterpret_cast<const uint8_t*>(str),
                                    v8::NewStringType::kInternalized)
      .ToLocalChecked();
}

inline void v8_set_property(v8::Local<v8::Object> target,
                            v8::Local<v8::Name> property,
                            v8::Local<v8::Value> value) {
  target->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), property, value);
}
inline void v8_set_property(
    v8::Local<v8::ObjectTemplate> target,
    v8::Local<v8::Name> property,
    v8::Local<v8::Data> value,
    v8::PropertyAttribute attributes = v8::PropertyAttribute::None) {
  target->Set(property, value, attributes);
}

struct ConstantValueItem {
  const char* name{""};
  int value{0};
  v8::PropertyAttribute propertyAttribute;
};
struct AttributeItem {
  const char* name{""};
  v8::FunctionCallback get_callback{nullptr};
  v8::FunctionCallback set_callback{nullptr};
  v8::PropertyAttribute propertyAttribute;
  v8::Local<v8::Signature> signature;
  v8::SideEffectType sideEffectType{v8::SideEffectType::kHasSideEffect};
};
struct OperationItem {
  const char* name{""};
  int length{0};
  v8::FunctionCallback callback{nullptr};
  v8::PropertyAttribute propertyAttribute;
  v8::Local<v8::Signature> signature;
  v8::SideEffectType sideEffectType{v8::SideEffectType::kHasSideEffect};
};

void v8_defineConstantValues(v8::Isolate* isolate,
                             v8::Local<v8::ObjectTemplate> target,
                             std::span<ConstantValueItem> configs);
void v8_defineAttributes(v8::Isolate* isolate,
                         v8::Local<v8::ObjectTemplate> target,
                         std::span<AttributeItem> configs);
void v8_defineOperations(v8::Isolate* isolate,
                         v8::Local<v8::ObjectTemplate> target,
                         std::span<OperationItem> configs);

v8::Local<v8::FunctionTemplate> v8_function(
    v8::FunctionCallback callback = nullptr,
    const char* name = "",
    int length = 0,
    bool isConstruct = false);

v8::Local<v8::FunctionTemplate> v8_getter(
    const char* name = "",
    v8::FunctionCallback callback = nullptr,
    v8::Local<v8::Signature> signature = v8::Local<v8::Signature>());
v8::Local<v8::FunctionTemplate> v8_setter(
    const char* name = "",
    v8::FunctionCallback callback = nullptr,
    v8::Local<v8::Signature> signature = v8::Local<v8::Signature>());
v8::Local<v8::FunctionTemplate> v8_operation(
    const char* name = "",
    int length = 0,
    v8::FunctionCallback callback = nullptr,
    v8::Local<v8::Signature> signature = v8::Local<v8::Signature>());

}  // namespace svm
