#include "isolate/external_data.h"

namespace svm {

CopyData::CopyData(v8::Local<v8::Context> context, v8::Local<v8::Value> value) {
  v8::Isolate* isolate{context->GetIsolate()};
  v8::HandleScope handle_scope{isolate};
  v8::Context::Scope context_scope{context};

  v8::ValueSerializer serializer{isolate};
  serializer.WriteHeader();
  if (serializer.WriteValue(context, value).FromMaybe(false)) {
    buffer_ = serializer.Release();
  }
}
CopyData::~CopyData() {
  Clear();
}

CopyData::CopyData(CopyData&& copy_data) noexcept {
  buffer_ = std::exchange(copy_data.buffer_, {});
}
CopyData& CopyData::operator=(CopyData&& copy_data) {
  buffer_ = std::exchange(copy_data.buffer_, {});
  return *this;
}

void CopyData::Serializer(v8::Local<v8::Context> context,
                          v8::Local<v8::Value> value) {
  v8::Isolate* isolate{context->GetIsolate()};
  v8::HandleScope handle_scope{isolate};
  v8::Context::Scope context_scope{context};

  v8::ValueSerializer serializer{isolate};
  serializer.WriteHeader();
  if (serializer.WriteValue(context, value).FromMaybe(false)) {
    Clear();
    buffer_ = serializer.Release();
  }
}

v8::Local<v8::Value> CopyData::Deserializer(
    v8::Local<v8::Context> context) const {
  if (IsEmpty()) {
    return {};
  }

  v8::Isolate* isolate{context->GetIsolate()};
  v8::TryCatch try_catch{isolate};
  v8::ValueDeserializer deserializer{isolate, buffer_.first, buffer_.second};
  deserializer.ReadHeader(context);
  v8::Local<v8::Value> result{};
  if (!deserializer.ReadValue(context).ToLocal(&result)) {
    result = try_catch.Exception();
    try_catch.Reset();
  }
  return result;
}

void CopyData::Clear() {
  if (!IsEmpty()) {
    DataDelegate data_delegate;
    data_delegate.FreeBufferMemory(buffer_.first);
    buffer_ = {};
  }
}

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
v8::Local<v8::Value> ExternalData::DeserializerSync(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    std::pair<uint8_t*, size_t>& buffer) {
  v8::TryCatch try_catch{isolate};
  v8::ValueDeserializer deserializer{isolate, buffer.first, buffer.second};
  deserializer.ReadHeader(context);
  v8::Local<v8::Value> result{};
  if (!deserializer.ReadValue(context).ToLocal(&result)) {
    result = try_catch.Exception();
    try_catch.Reset();
  }
  FreeBufferMemory(buffer);
  return result;
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
v8::Local<v8::Value> ExternalData::DeserializerAsync(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    std::pair<uint8_t*, size_t>& buff) {
  return {};
}

void ExternalData::FreeBufferMemory(std::pair<uint8_t*, size_t>& buffer) {
  DataDelegate data_delegate;
  data_delegate.FreeBufferMemory(buffer.first);
}

}  // namespace svm
