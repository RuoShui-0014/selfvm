#pragma once

#include <v8.h>

#include <span>

#include "../isolate/script_wrappable.h"
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

inline v8::Local<v8::String> toString(v8::Isolate* isolate, std::string str) {
  return v8::String::NewFromUtf8(isolate, str.c_str()).ToLocalChecked();
}

enum class Dependence { kConstruct, kPrototype, kInstance };
struct ConstructConfig {
  const char* name;
  int length = 0;
  v8::FunctionCallback callback = nullptr;
};
struct AttributeConfig {
  const char* name = "";
  v8::FunctionCallback get_callback = nullptr;
  v8::FunctionCallback set_callback = nullptr;
  v8::PropertyAttribute propertyAttribute;
  Dependence dep;
  v8::SideEffectType sideEffectType = v8::SideEffectType::kHasNoSideEffect;
};
struct OperationConfig {
  const char* name = "";
  int length = 0;
  v8::FunctionCallback callback = nullptr;
  v8::PropertyAttribute propertyAttribute;
  Dependence dep;
  v8::SideEffectType sideEffectType = v8::SideEffectType::kHasNoSideEffect;
};
struct ExposedConstructConfig {
  const char* name = "";
  v8::AccessorNameGetterCallback callback = nullptr;
  Dependence dep;
};

inline void InstallConstructor(v8::Isolate* isolate,
                               v8::Local<v8::Template> interface_template,
                               const ConstructConfig& constructor) {
  v8::Local<v8::FunctionTemplate> constructor_template =
      interface_template.As<v8::FunctionTemplate>();

  constructor_template->SetLength(constructor.length);
  constructor_template->SetClassName(toString(isolate, constructor.name));
  constructor_template->SetCallHandler(constructor.callback);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(2);
  constructor_template->PrototypeTemplate()->Set(
      v8::Symbol::GetToStringTag(isolate), toString(isolate, constructor.name));
}

inline void InstallAttributes(v8::Isolate* isolate,
                              v8::Local<v8::Template> interface_template,
                              v8::Local<v8::Signature> signature,
                              std::span<const AttributeConfig> attributes) {
  v8::Local<v8::FunctionTemplate> constructor_template =
      interface_template.As<v8::FunctionTemplate>();
  v8::Local<v8::ObjectTemplate> prototype =
      constructor_template->PrototypeTemplate();
  v8::Local<v8::ObjectTemplate> instance =
      constructor_template->InstanceTemplate();

  for (const auto& config : attributes) {
    v8::Local<v8::FunctionTemplate> get, set;
    if (config.get_callback) {
      get = v8::FunctionTemplate::New(
          isolate, config.get_callback, v8::Local<v8::Value>(), signature, 0,
          v8::ConstructorBehavior::kThrow, config.sideEffectType);
      get->SetClassName(
          toString(isolate, std::string{"get "} + std::string{config.name}));
    }
    if (config.set_callback) {
      set = v8::FunctionTemplate::New(
          isolate, config.set_callback, v8::Local<v8::Value>(), signature, 1,
          v8::ConstructorBehavior::kThrow, config.sideEffectType);
      set->SetClassName(
          toString(isolate, std::string{"set "} + std::string{config.name}));
    }
    if (config.dep == Dependence ::kPrototype) {
      prototype->SetAccessorProperty(toString(isolate, config.name), get, set,
                                     config.propertyAttribute);
    } else if (config.dep == Dependence ::kInstance) {
      instance->SetAccessorProperty(toString(isolate, config.name), get, set,
                                    config.propertyAttribute);
    } else if (config.dep == Dependence ::kConstruct) {
      constructor_template->SetAccessorProperty(
          toString(isolate, config.name), get, set, config.propertyAttribute);
    }
  }
}

inline void InstallOperations(v8::Isolate* isolate,
                              v8::Local<v8::Template> interface_template,
                              v8::Local<v8::Signature> signature,
                              std::span<const OperationConfig> operations) {
  v8::Local<v8::FunctionTemplate> constructor_template =
      interface_template.As<v8::FunctionTemplate>();
  v8::Local<v8::ObjectTemplate> prototype =
      constructor_template->PrototypeTemplate();
  v8::Local<v8::ObjectTemplate> instance =
      constructor_template->InstanceTemplate();

  for (const auto& config : operations) {
    v8::Local<v8::FunctionTemplate> act = v8::FunctionTemplate::New(
        isolate, config.callback, v8::Local<v8::Value>(), signature,
        config.length, v8::ConstructorBehavior::kThrow, config.sideEffectType);
    act->SetClassName(toString(isolate, config.name));

    if (config.dep == Dependence ::kPrototype) {
      prototype->Set(isolate, config.name, act, config.propertyAttribute);
    } else if (config.dep == Dependence ::kInstance) {
      instance->Set(isolate, config.name, act, config.propertyAttribute);
    } else if (config.dep == Dependence ::kConstruct) {
      interface_template->Set(isolate, config.name, act,
                              config.propertyAttribute);
    }
  }
}

inline void InstallExposedConstructs(
    v8::Isolate* isolate,
    v8::Local<v8::Template> interface_template,
    std::span<const ExposedConstructConfig> exposedConstructs) {
  v8::Local<v8::FunctionTemplate> constructor_template =
      interface_template.As<v8::FunctionTemplate>();
  v8::Local<v8::ObjectTemplate> prototype_template =
      constructor_template->PrototypeTemplate();
  v8::Local<v8::ObjectTemplate> instance_template =
      constructor_template->InstanceTemplate();

  for (const auto& config : exposedConstructs) {
    v8::Local<v8::String> name = toString(isolate, config.name);

    if (config.dep == Dependence ::kInstance) {
      instance_template->SetLazyDataProperty(
          name, config.callback, v8::Local<v8::Value>(), v8::DontEnum,
          v8::SideEffectType::kHasNoSideEffect);
    }
    if (config.dep == Dependence ::kPrototype) {
      prototype_template->SetLazyDataProperty(
          name, config.callback, v8::Local<v8::Value>(), v8::DontEnum,
          v8::SideEffectType::kHasNoSideEffect);
    } else if (config.dep == Dependence ::kConstruct) {
      interface_template->SetLazyDataProperty(
          name, config.callback, v8::Local<v8::Value>(), v8::DontEnum,
          v8::SideEffectType::kHasNoSideEffect);
    }
  }
}

template <typename T>
class RemoteHandle;

template <typename T>
class RemoteHandle {
 public:
  RemoteHandle(v8::Isolate* isolate, v8::Local<T> value)
      : isolate_(isolate), handle_{isolate, value} {}
  RemoteHandle(RemoteHandle& other) noexcept
      : isolate_(other.isolate_), handle_(other.handle_.Pass()) {}
  RemoteHandle(RemoteHandle&& other) noexcept
      : isolate_(std::move(other.isolate_)),
        handle_{std::move(other.handle_)} {}

  v8::Local<T> Get(v8::Isolate* isolate) { return handle_.Get(isolate); }

  void Clear() { handle_.Clear(); }

  void Reset() { handle_.Clear(); }

  void Reset(v8::Isolate* isolate, v8::Local<T> value) {
    handle_.Clear();
    handle_.Reset(isolate, value);
  }

  ~RemoteHandle() { handle_.Clear(); }

 private:
  v8::Isolate* isolate_;
  v8::Global<T> handle_;
};

template <>
class RemoteHandle<v8::Context> {
 public:
  explicit RemoteHandle(v8::Local<v8::Context> value)
      : isolate_(value->GetIsolate()), handle_{isolate_, value} {}
  RemoteHandle(v8::Isolate* isolate, v8::Local<v8::Context> value)
      : isolate_(isolate), handle_{isolate, value} {}
  RemoteHandle(RemoteHandle& other) noexcept
      : isolate_(other.isolate_), handle_(other.handle_.Pass()) {}
  RemoteHandle(RemoteHandle&& other) noexcept
      : isolate_(other.isolate_), handle_{std::move(other.handle_)} {}

  v8::Local<v8::Context> Get() const {
    v8::HandleScope scope(isolate_);
    return handle_.Get(isolate_);
  }
  v8::Local<v8::Context> Get(v8::Isolate* isolate) const {
    return handle_.Get(isolate);
  }

  v8::Isolate* GetIsolate() const { return isolate_; }

  void Clear() { handle_.Clear(); }

  void Reset() { handle_.Clear(); }

  void Reset(v8::Isolate* isolate, v8::Local<v8::Context> value) {
    handle_.Clear();
    handle_.Reset(isolate, value);
  }

  ~RemoteHandle() { handle_.Clear(); }

 private:
  v8::Isolate* isolate_;
  v8::Global<v8::Context> handle_;
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

template <typename Info, typename T>
void V8SetReturnValue(Info& info, T* value) {
  info.GetReturnValue().Set(value->V8Object(info.GetIsolate()));
}

}  // namespace svm
