#pragma once

#include <v8.h>

#include "../isolate/wrapper_type_info.h"

namespace svm {

inline v8::Local<v8::String> toString(const char* str) {
  return v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), str)
      .ToLocalChecked();
}

inline v8::Local<v8::String> toString(v8::Isolate* isolate, const char* str) {
  return v8::String::NewFromUtf8(isolate, str).ToLocalChecked();
}

inline v8::Local<v8::String> toString(const std::string& str) {
  return v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), str.c_str())
      .ToLocalChecked();
}

inline v8::Local<v8::String> toString(v8::Isolate* isolate, std::string& str) {
  return v8::String::NewFromUtf8(isolate, str.c_str()).ToLocalChecked();
}

enum class Dependence { kConstruct, kPrototype, kInstance };
struct ConstructItem {
  const char* name;
  int length = 0;
  v8::FunctionCallback callback = nullptr;
};
struct AttributeItem {
  const char* name = "";
  v8::FunctionCallback get_callback = nullptr;
  v8::FunctionCallback set_callback = nullptr;
  v8::PropertyAttribute propertyAttribute;
  Dependence dep;
  v8::SideEffectType sideEffectType = v8::SideEffectType::kHasNoSideEffect;
};
struct OperationItem {
  const char* name = "";
  int length = 0;
  v8::FunctionCallback callback = nullptr;
  v8::PropertyAttribute propertyAttribute;
  Dependence dep;
  v8::SideEffectType sideEffectType = v8::SideEffectType::kHasNoSideEffect;
};
struct ExposedConstructItem {
  const char* name = "";
  v8::AccessorNameGetterCallback callback = nullptr;
  Dependence dep;
};

inline void InstallConstructor(v8::Isolate* isolate,
                               v8::Local<v8::Template> interface_template,
                               const ConstructItem& constructor) {
  v8::Local<v8::FunctionTemplate> constructor_template =
      interface_template.As<v8::FunctionTemplate>();

  constructor_template->SetLength(constructor.length);
  constructor_template->SetClassName(toString(isolate, constructor.name));
  constructor_template->SetCallHandler(constructor.callback);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(2);
  constructor_template->PrototypeTemplate()->Set(
      v8::Symbol::GetToStringTag(isolate), toString(isolate, constructor.name));
}

template <size_t N>
void InstallAttributes(v8::Isolate* isolate,
                       v8::Local<v8::Template> interface_template,
                       const AttributeItem (&attributes)[N],
                       v8::Local<v8::Signature> signature) {
  v8::Local<v8::FunctionTemplate> constructor_template =
      interface_template.As<v8::FunctionTemplate>();
  v8::Local<v8::ObjectTemplate> prototype =
      constructor_template->PrototypeTemplate();
  v8::Local<v8::ObjectTemplate> instance =
      constructor_template->InstanceTemplate();

  for (const auto& attr : attributes) {
    v8::Local<v8::FunctionTemplate> get, set;
    if (attr.get_callback) {
      get = v8::FunctionTemplate::New(
          isolate, attr.get_callback, v8::Local<v8::Value>(), signature, 0,
          v8::ConstructorBehavior::kThrow, attr.sideEffectType);
      get->SetClassName(toString(std::string{"get "} + std::string{attr.name}));
    }
    if (attr.set_callback) {
      set = v8::FunctionTemplate::New(
          isolate, attr.set_callback, v8::Local<v8::Value>(), signature, 1,
          v8::ConstructorBehavior::kThrow, attr.sideEffectType);
      set->SetClassName(toString(std::string{"set "} + std::string{attr.name}));
    }
    if (attr.dep == Dependence ::kPrototype) {
      prototype->SetAccessorProperty(toString(attr.name), get, set,
                                     attr.propertyAttribute);
    } else if (attr.dep == Dependence ::kInstance) {
      instance->SetAccessorProperty(toString(attr.name), get, set,
                                    attr.propertyAttribute);
    } else if (attr.dep == Dependence ::kConstruct) {
      constructor_template->SetAccessorProperty(toString(attr.name), get, set,
                                                attr.propertyAttribute);
    }
  }
}

template <size_t N>
void InstallOperations(v8::Isolate* isolate,
                       v8::Local<v8::Template> interface_template,
                       const OperationItem (&operations)[N],
                       v8::Local<v8::Signature> signature) {
  v8::Local<v8::FunctionTemplate> constructor_template =
      interface_template.As<v8::FunctionTemplate>();
  v8::Local<v8::ObjectTemplate> prototype =
      constructor_template->PrototypeTemplate();
  v8::Local<v8::ObjectTemplate> instance =
      constructor_template->InstanceTemplate();

  for (const auto& opera : operations) {
    v8::Local<v8::FunctionTemplate> act = v8::FunctionTemplate::New(
        isolate, opera.callback, v8::Local<v8::Value>(), signature,
        opera.length, v8::ConstructorBehavior::kThrow, opera.sideEffectType);
    act->SetClassName(toString(opera.name));

    if (opera.dep == Dependence ::kPrototype) {
      prototype->Set(isolate, opera.name, act, opera.propertyAttribute);
    } else if (opera.dep == Dependence ::kInstance) {
      instance->Set(isolate, opera.name, act, opera.propertyAttribute);
    } else if (opera.dep == Dependence ::kConstruct) {
      interface_template->Set(isolate, opera.name, act,
                              opera.propertyAttribute);
    }
  }
}

template <size_t N>
void InstallExposedConstructs(
    v8::Isolate* isolate,
    v8::Local<v8::Template> interface_template,
    const ExposedConstructItem (&exposedConstructs)[N]) {
  v8::Local<v8::FunctionTemplate> constructor_template =
      interface_template.As<v8::FunctionTemplate>();
  v8::Local<v8::ObjectTemplate> prototype_template =
      constructor_template->PrototypeTemplate();
  v8::Local<v8::ObjectTemplate> instance_template =
      constructor_template->InstanceTemplate();

  for (const auto& config : exposedConstructs) {
    v8::Local<v8::String> name = toString(isolate, config.name);
    instance_template->SetLazyDataProperty(
        name, config.callback, v8::Local<v8::Value>(), v8::DontEnum,
        v8::SideEffectType::kHasNoSideEffect);
  }
}

template <typename T>
class RemoteHandle {
 public:
  RemoteHandle(v8::Isolate* isolate, v8::Local<T> value)
      : handle_{isolate, value} {}
  explicit RemoteHandle(v8::Global<T>&& value) : handle_{std::move(value)} {}
  RemoteHandle(RemoteHandle&& other) noexcept
      : handle_{std::move(other.handle_)} {}

  v8::Local<T> Get(v8::Isolate* isolate) { return handle_.Get(isolate); }

  void Clear() { handle_.Clear(); }

  void Reset() { handle_.Clear(); }

  void Reset(v8::Isolate* isolate, v8::Local<T> value) {
    handle_.Clear();
    handle_.Reset(isolate, value);
  }

  ~RemoteHandle() { handle_.Clear(); }

 private:
  v8::Global<T> handle_;
};

class V8CtxScope {
 public:
  V8CtxScope(v8::Isolate* isolate, v8::Local<v8::Context> context)
      : isolate_scope_(isolate),
        handle_scope_(isolate),
        context_scope_(context),
        isolate_(isolate),
        context_(context) {}
  V8CtxScope(v8::Isolate* isolate, const v8::Global<v8::Context>* context)
      : isolate_scope_(isolate),
        handle_scope_(isolate),
        context_scope_(context->Get(isolate)),
        isolate_(isolate),
        context_(context->Get(isolate)) {}

  v8::Isolate* GetIsolate() const { return isolate_; }
  v8::Local<v8::Context> GetContext() const { return context_; }

 private:
  v8::Isolate::Scope isolate_scope_;
  v8::HandleScope handle_scope_;
  v8::Context::Scope context_scope_;
  v8::Isolate* isolate_;
  v8::Local<v8::Context> context_;
};

inline v8::Local<v8::Value> GetExposedInterfaceObject(
    v8::Isolate* isolate,
    v8::Local<v8::Object> creation_context,
    const WrapperTypeInfo* wrapper_type_info) {
  v8::Local<v8::FunctionTemplate> ftmp =
      wrapper_type_info->GetV8ClassTemplate(isolate).As<v8::FunctionTemplate>();
  v8::Local<v8::Context> context =
      creation_context->GetCreationContextChecked();
  return ftmp->GetFunction(context).ToLocalChecked();
}

}  // namespace svm
