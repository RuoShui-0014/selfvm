//
// Created by ruoshui on 25-7-4.
//

#pragma once

#include "../utils/utils.h"

namespace svm {

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
  static std::pair<uint8_t*, size_t> SerializerAsync(const SourceData& source);

  static v8::Local<v8::Value> DeserializerSync(
      v8::Isolate* isolate,
      v8::Local<v8::Context> context,
      std::pair<uint8_t*, size_t>& buff);
  static v8::Local<v8::Value> DeserializerAsync(
      v8::Isolate* isolate,
      v8::Local<v8::Context> context,
      std::pair<uint8_t*, size_t>& buff);
};

}  // namespace svm
