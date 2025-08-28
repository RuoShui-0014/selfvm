#include "script_wrappable.h"

#include <node.h>

namespace cppgc::internal {
AtomicEntryFlag WriteBarrier::write_barrier_enabled_;
}

namespace svm {

ScriptWrappable::~ScriptWrappable() = default;

void ScriptWrappable::Wrap(const v8::Local<v8::Object> object,
                           ScriptWrappable* wrappable) {
  assert(wrappable->wrapper_.IsEmpty());
  assert(object->InternalFieldCount() > 1);
  v8::Isolate* isolate{object->GetIsolate()};
  node::SetCppgcReference(isolate, object, wrappable);
  wrappable->wrapper_.Reset(isolate, object);
}

}  // namespace svm
