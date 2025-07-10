#include "external_data.h"

namespace svm {

std::pair<uint8_t*, size_t> ExternalData::CopySync(SourceData& source) {
  v8::Isolate::Scope isolate_scope(source.isolate);
  v8::HandleScope scope(source.isolate);
  v8::Context::Scope context{source.context};

  v8::ValueSerializer serializer(source.isolate);
  serializer.WriteHeader();
  if (!serializer.WriteValue(source.context, source.result).FromMaybe(false)) {
    return {};
  }

  return serializer.Release();
}

std::pair<uint8_t*, size_t> ExternalData::CopyAsync(SourceData& source) {
  v8::Isolate::Scope isolate_scope(source.isolate);
  v8::HandleScope scope(source.isolate);
  v8::Context::Scope context{source.context};
  v8::ValueSerializer serializer(source.isolate);
  serializer.WriteHeader();
  if (!serializer.WriteValue(source.context, source.result).FromMaybe(false)) {
    return {};
  }

  return serializer.Release();
}

}  // namespace svm
