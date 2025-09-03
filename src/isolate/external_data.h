#pragma once

#include <v8.h>

namespace svm {

class DataDelegate final : public v8::ValueSerializer::Delegate {
 public:
  DataDelegate() = default;
  ~DataDelegate() override = default;

  void ThrowDataCloneError(v8::Local<v8::String> msg) override {}
};

class CopyData {
 public:
  CopyData() = default;
  CopyData(v8::Local<v8::Context> context, v8::Local<v8::Value> value);
  ~CopyData();

  CopyData(const CopyData&) = delete;
  CopyData& operator=(const CopyData&) = delete;

  CopyData(CopyData&&) noexcept;
  CopyData& operator=(CopyData&&);

  void Serializer(v8::Local<v8::Context> context, v8::Local<v8::Value> value);
  v8::Local<v8::Value> Deserializer(v8::Local<v8::Context> context) const;
  bool IsEmpty() const { return buffer_.first == nullptr; }
  size_t GetSize() const { return buffer_.second; }
  void Clear();

 private:
  std::pair<uint8_t*, size_t> buffer_;
};

class ExternalData {
 public:
  struct SourceData {
    v8::Isolate* isolate;
    v8::Local<v8::Context> context;
    v8::Local<v8::Value> result;
  };
  struct TargetData {
    v8::Isolate* isolate;
    v8::Local<v8::Context> context;
    v8::Local<v8::Value>* result;
  };

  static std::pair<uint8_t*, size_t> SerializerSync(const SourceData& source);
  static v8::Local<v8::Value> DeserializerSync(
      v8::Isolate* isolate,
      v8::Local<v8::Context> context,
      std::pair<uint8_t*, size_t>& buffer);

  static std::pair<uint8_t*, size_t> SerializerAsync(const SourceData& source);
  static v8::Local<v8::Value> DeserializerAsync(
      v8::Isolate* isolate,
      v8::Local<v8::Context> context,
      std::pair<uint8_t*, size_t>& buff);

  static void FreeBufferMemory(std::pair<uint8_t*, size_t>& buffer);
};

}  // namespace svm
