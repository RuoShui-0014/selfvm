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

  static std::pair<uint8_t*, size_t> CopySync(SourceData& source);
  static std::pair<uint8_t*, size_t> CopyAsync(SourceData& source);
};

}  // namespace svm
