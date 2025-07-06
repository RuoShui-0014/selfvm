//
// Created by ruoshui on 25-7-3.
//

#pragma once
#include <v8-isolate.h>

#include "per_isolate_data.h"
#include "scheduler.h"

namespace svm {

class IsolateHolder {
 public:
  explicit IsolateHolder(v8::Isolate* parent_isolate,
                         size_t memory_limit = 128);
  ~IsolateHolder();

  v8::Isolate* GetIsolate() const { return isolate_self_; }
  v8::Isolate* GetParentIsolate() const { return isolate_parent_; }
  Scheduler* GetScheduler() const { return scheduler_self_.get(); }
  Scheduler* GetParentScheduler() const { return scheduler_parent_.get(); }

  v8::Local<v8::Context> NewContext();

 private:
  v8::Isolate* isolate_parent_;
  v8::Isolate* isolate_self_;

  std::shared_ptr<Scheduler> scheduler_parent_;
  std::shared_ptr<Scheduler> scheduler_self_;

  std::unique_ptr<PerIsolateData> per_isolate_data_;
  std::unique_ptr<v8::ArrayBuffer::Allocator> allocator_;

  size_t memory_limit = 128;
};

}  // namespace svm
