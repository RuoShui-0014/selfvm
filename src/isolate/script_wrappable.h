#pragma once

#include <node.h>

#include <cassert>

#include "cppgc/allocation.h"
#include "cppgc/garbage-collected.h"
#include "cppgc/visitor.h"
#include "v8-cppgc.h"
#include "v8-traced-handle.h"

namespace svm {

class ScriptWrappable : public cppgc::GarbageCollected<ScriptWrappable> {
 public:
  virtual ~ScriptWrappable();

  static void Wrap(v8::Local<v8::Object> object, ScriptWrappable* wrappable);

  template <class T>
  static T* Unwrap(const v8::Local<v8::Object> object) {
    assert(!object.IsEmpty() && "Unwrap target is empty.");
    assert(object->InternalFieldCount() > 1 &&
           "Unwrap target is internal field count not >1.");

    const v8::Isolate* isolate{object->GetIsolate()};
    const v8::WrapperDescriptor descriptor{
        isolate->GetCppHeap()->wrapper_descriptor()};
    auto* ptr{static_cast<ScriptWrappable*>(
        object->GetAlignedPointerFromInternalField(
            descriptor.wrappable_instance_index))};
    return static_cast<T*>(ptr);
  }

  v8::Local<v8::Object> V8Object(v8::Isolate* isolate) const {
    return v8::Local<v8::Object>::New(isolate, wrapper_);
  }

  virtual void Trace(cppgc::Visitor* visitor) const {
    visitor->Trace(wrapper_);
  }

 protected:
  ScriptWrappable() = default;

 private:
  v8::TracedReference<v8::Object> wrapper_;
};

/**
 * new object in v8 cppgc pool
 */
template <typename T, typename... Args>
T* MakeCppGcObject_1(Args&&... args) {
  static_assert(std::is_base_of_v<ScriptWrappable, T>,
                "Cppgc class have to base of ScriptWrappable.");

  return cppgc::MakeGarbageCollected<T>(
      v8::Isolate::GetCurrent()->GetCppHeap()->GetAllocationHandle(),
      std::forward<Args>(args)...);
}
template <typename T, typename... Args>
T* MakeCppGcObject_2(v8::Isolate* isolate, Args&&... args) {
  static_assert(std::is_base_of_v<ScriptWrappable, T>,
                "Cppgc class have to base of ScriptWrappable.");

  return cppgc::MakeGarbageCollected<T>(
      isolate->GetCppHeap()->GetAllocationHandle(),
      std::forward<Args>(args)...);
}

enum class GC { kCurrent, kSpecified };
template <GC e, typename T, typename... Args>
T* MakeCppGcObject(Args&&... args) {
  static_assert(std::is_base_of_v<ScriptWrappable, T>,
                "Cppgc class have to base of ScriptWrappable.");

  if constexpr (e == GC::kCurrent) {
    return MakeCppGcObject_1<T>(std::forward<Args>(args)...);
  } else {
    return MakeCppGcObject_2<T>(std::forward<Args>(args)...);
  }
}

}  // namespace svm
