#include "utils.h"

namespace svm {

void v8_defineConstantValues(v8::Isolate* isolate,
                             v8::Local<v8::ObjectTemplate> target,
                             std::span<ConstantValueItem> configs) {
  for (const auto& [name, value, propertyAttribute] : configs) {
    target->Set(isolate, name, v8::Integer::New(isolate, value),
                propertyAttribute);
  }
}
void v8_defineAttributes(v8::Isolate* isolate,
                         v8::Local<v8::ObjectTemplate> target,
                         std::span<AttributeItem> configs) {
  for (const auto& [name, get_callback, set_callback, propertyAttribute,
                    signature, sideEffectType] : configs) {
    v8::Local<v8::FunctionTemplate> get_fun_temp, set_fun_temp;
    std::string name_{name};
    if (get_callback) {
      get_fun_temp = v8::FunctionTemplate::New(
          isolate, get_callback, {}, signature, 0,
          v8::ConstructorBehavior::kThrow, sideEffectType);
      std::string get_name = std::string("get ") + name_;
      get_fun_temp->SetClassName(v8_str(get_name.data()));
    }
    if (set_callback) {
      set_fun_temp = v8::FunctionTemplate::New(
          isolate, set_callback, {}, signature, 1,
          v8::ConstructorBehavior::kThrow, sideEffectType);
      std::string set_name = std::string("set ") + name_;
      set_fun_temp->SetClassName(v8_str(set_name.data()));
    }
    target->SetAccessorProperty(v8_str(name), get_fun_temp, set_fun_temp,
                                propertyAttribute);
  }
}
void v8_defineOperations(v8::Isolate* isolate,
                         v8::Local<v8::ObjectTemplate> target,
                         std::span<OperationItem> configs) {
  for (const auto& [name, length, callback, propertyAttribute, signature,
                    sideEffectType] : configs) {
    const v8::Local fun_temp{v8::FunctionTemplate::New(
        isolate, callback, {}, signature, length,
        v8::ConstructorBehavior::kThrow, sideEffectType)};
    fun_temp->SetClassName(v8_str(name));
    target->Set(isolate, name, fun_temp, propertyAttribute);
  }
}

v8::Local<v8::FunctionTemplate> v8_function(v8::FunctionCallback callback,
                                            const char* name,
                                            int length,
                                            bool isConstruct) {
  const v8::Local func_temp{v8::FunctionTemplate::New(
      v8::Isolate::GetCurrent(), callback, {}, {}, length,
      isConstruct ? v8::ConstructorBehavior::kAllow
                  : v8::ConstructorBehavior::kThrow)};
  func_temp->SetClassName(v8_str(name));

  return func_temp;
}

v8::Local<v8::FunctionTemplate> v8_getter(const char* name,
                                          v8::FunctionCallback callback,
                                          v8::Local<v8::Signature> signature) {
  const v8::Local func_temp{
      v8::FunctionTemplate::New(v8::Isolate::GetCurrent(), callback, {},
                                signature, 0, v8::ConstructorBehavior::kThrow)};
  func_temp->SetClassName(v8_str(name));

  return func_temp;
}
v8::Local<v8::FunctionTemplate> v8_setter(const char* name,
                                          v8::FunctionCallback callback,
                                          v8::Local<v8::Signature> signature) {
  const v8::Local func_temp{
      v8::FunctionTemplate::New(v8::Isolate::GetCurrent(), callback, {},
                                signature, 1, v8::ConstructorBehavior::kThrow)};
  func_temp->SetClassName(v8_str(name));

  return func_temp;
}
v8::Local<v8::FunctionTemplate> v8_operation(
    const char* name,
    int length,
    v8::FunctionCallback callback,
    v8::Local<v8::Signature> signature) {
  const v8::Local func_temp{v8::FunctionTemplate::New(
      v8::Isolate::GetCurrent(), callback, {}, signature, length,
      v8::ConstructorBehavior::kThrow)};
  func_temp->SetClassName(v8_str(name));
  return func_temp;
}
}  // namespace svm
