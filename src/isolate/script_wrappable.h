#pragma once

#include <node.h>

#include <cassert>

#include "cppgc/allocation.h"
#include "cppgc/garbage-collected.h"
#include "cppgc/visitor.h"
#include "v8-cppgc.h"
#include "v8-traced-handle.h"
#include "v8.h"

namespace svm {

class ScriptWrappable : public cppgc::GarbageCollected<ScriptWrappable> {
 public:
  virtual ~ScriptWrappable();

  static void Wrap(v8::Local<v8::Object> object, ScriptWrappable* wrappable);

  template <class T>
  static T* Unwrap(v8::Local<v8::Object> object) {
    assert(!object.IsEmpty());
    assert(object->InternalFieldCount() > 1);
    v8::Isolate* isolate = object->GetIsolate();
    v8::WrapperDescriptor descriptor =
        isolate->GetCppHeap()->wrapper_descriptor();
    ScriptWrappable* ptr = static_cast<ScriptWrappable*>(
        object->GetAlignedPointerFromInternalField(
            descriptor.wrappable_instance_index));

    return static_cast<T*>(ptr);
  }

  inline v8::Local<v8::Object> V8Object(v8::Isolate* isolate) {
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
