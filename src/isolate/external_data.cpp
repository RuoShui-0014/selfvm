#include "external_data.h"

namespace svm {

bool ExternalData::Copy(Data& target, Data& source) {
  std::pair<uint8_t*, size_t> buffer;

  {
    v8::Locker locker(source.isolate);
    V8Scope scope(source.isolate, source.context);
    v8::ValueSerializer serializer(source.isolate);
    if (!serializer.WriteValue(source.context, source.value).FromMaybe(false)) {
      return false;
    }
    buffer = serializer.Release();
  }

  {
    v8::Locker locker(target.isolate);
    V8Scope scope(target.isolate, target.context);
    v8::ValueDeserializer deserializer(target.isolate, buffer.first,
                                       buffer.second);
    if (!deserializer.ReadValue(target.context).ToLocal(&target.value)) {
      return false;
    }
  }

  return true;
}

}  // namespace svm
