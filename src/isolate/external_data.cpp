#include "external_data.h"

namespace svm {

std::pair<uint8_t*, size_t> ExternalData::SerializerSync(
    const SourceData& source) {
  v8::Isolate::Scope isolate_scope{source.isolate};
  v8::HandleScope scope{source.isolate};
  v8::Context::Scope context{source.context};

  v8::ValueSerializer serializer{source.isolate};
  serializer.WriteHeader();
  if (!serializer.WriteValue(source.context, source.result).FromMaybe(false)) {
    return {};
  }

  return serializer.Release();
}

std::pair<uint8_t*, size_t> ExternalData::SerializerAsync(
    const SourceData& source) {
  v8::Isolate::Scope isolate_scope{source.isolate};
  v8::HandleScope scope{source.isolate};
  v8::Context::Scope context{source.context};
  v8::ValueSerializer serializer{source.isolate};
  serializer.WriteHeader();
  if (!serializer.WriteValue(source.context, source.result).FromMaybe(false)) {
    return {};
  }

  return serializer.Release();
}
v8::Local<v8::Value> ExternalData::DeserializerSync(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    std::pair<uint8_t*, size_t>& buff) {
  v8::TryCatch try_catch{isolate};
  v8::ValueDeserializer deserializer{isolate, buff.first, buff.second};
  deserializer.ReadHeader(context);
  v8::Local<v8::Value> result{};
  if (!deserializer.ReadValue(context).ToLocal(&result)) {
    result = try_catch.Exception();
    try_catch.Reset();
  }
  return result;
}

v8::Local<v8::Value> ExternalData::DeserializerAsync(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    std::pair<uint8_t*, size_t>& buff) {
  return {};
}

}  // namespace svm
