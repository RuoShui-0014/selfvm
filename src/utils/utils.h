#pragma once

#include <v8.h>

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
      prototype->Set(isolate, opera.name, act,
                     opera.propertyAttribute);
    } else if (opera.dep == Dependence ::kInstance) {
      instance->Set(isolate, opera.name, act,
                    opera.propertyAttribute);
    } else if (opera.dep == Dependence ::kConstruct) {
      interface_template->Set(isolate, opera.name, act,
                              opera.propertyAttribute);
    }
  }
}

/**
 * new object in v8 cppgc pool
 */
template <typename T, typename... Args>
T* MakeCppGcObject_1(Args&&... args) {
  return cppgc::MakeGarbageCollected<T>(
      v8::Isolate::GetCurrent()->GetCppHeap()->GetAllocationHandle(),
      std::forward<Args>(args)...);
}
template <typename T, typename... Args>
T* MakeCppGcObject_2(v8::Isolate* isolate, Args&&... args) {
  return cppgc::MakeGarbageCollected<T>(
      isolate->GetCppHeap()->GetAllocationHandle(),
      std::forward<Args>(args)...);
}

enum class GC { kCurrent, kSpecified };
template <GC e, typename T, typename... Args>
T* MakeCppGcObject(Args&&... args) {
  if constexpr (e == GC::kCurrent) {
    return MakeCppGcObject_1<T>(std::forward<Args>(args)...);
  } else {
    return MakeCppGcObject_2<T>(std::forward<Args>(args)...);
  }
}

}  // namespace svm
