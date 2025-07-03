//
// Created by ruoshui on 25-7-3.
//

#pragma once
#include <v8-isolate.h>

#include "scheduler.h"

namespace svm {

class IsolateHolder {
 public:
  explicit IsolateHolder(v8::Isolate* parent_isolate, size_t memory_limit = 128);
  ~IsolateHolder();

  v8::Isolate* GetIsolate() const { return self_isolate_; }
  v8::Isolate* GetParentIsolate() const { return parent_isolate_; }

  v8::Local<v8::Context> NewContext();

 private:
  v8::Isolate* parent_isolate_;
  v8::Isolate* self_isolate_;

  std::shared_ptr<Scheduler> scheduler_;
  std::unique_ptr<v8::ArrayBuffer::Allocator> allocator_;

  size_t memory_limit = 128;
};

}  // namespace svm
