//
// Created by ruoshui on 25-7-4.
//

#pragma once

#include "../utils/utils.h"

namespace svm {

class ExternalData {
 public:
  struct Data {
    v8::Isolate* isolate;
    v8::Local<v8::Context> context;
    v8::Local<v8::Value> value;
  };

  static bool Copy(Data& target, Data& source);
};

}  // namespace svm
