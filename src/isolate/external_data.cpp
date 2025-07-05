#include "external_data.h"

namespace svm {

bool ExternalData::CopySync(SourceData& source, TargetData& target) {
  std::pair<uint8_t*, size_t> buffer;

  {
    v8::Locker source_locker(source.isolate);
    v8::HandleScope source_scope(source.isolate);
    v8::Context::Scope context_scope{source.context};
    v8::ValueSerializer serializer(source.isolate);
    if (!serializer.WriteValue(source.context, source.result)
             .FromMaybe(false)) {
      return false;
    }
    buffer = serializer.Release();
  }

  {
    v8::Locker target_locker(target.isolate);
    v8::HandleScope target_scope(target.isolate);
    v8::Context::Scope context_scope{target.context};
    v8::ValueDeserializer deserializer(target.isolate, buffer.first,
                                       buffer.second);
    if (!deserializer.ReadValue(target.context).ToLocal(target.result)) {
      return false;
    }
  }

  return true;
}

std::pair<uint8_t*, size_t> ExternalData::CopyAsync(SourceData& source) {
  v8::Locker source_locker(source.isolate);
  v8::HandleScope source_scope(source.isolate);
  v8::Context::Scope context_scope{source.context};
  v8::ValueSerializer serializer(source.isolate);
  if (!serializer.WriteValue(source.context, source.result).FromMaybe(false)) {
    return std::pair<uint8_t*, size_t>{};
  }

  return serializer.Release();
}

}  // namespace svm
