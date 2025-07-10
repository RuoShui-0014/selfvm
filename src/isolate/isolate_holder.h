//
// Created by ruoshui on 25-7-3.
//

#pragma once
#include <v8-isolate.h>

#include "../utils/utils.h"
#include "per_isolate_data.h"
#include "scheduler.h"

namespace svm {

class IsolateHolder {
 public:
  explicit IsolateHolder(v8::Isolate* isolate_parent,
                         size_t memory_limit = 128);
  ~IsolateHolder();

  v8::Isolate* GetIsolate() const { return isolate_self_; }
  v8::Isolate* GetParentIsolate() const { return isolate_parent_; }
  Scheduler* GetScheduler() const { return scheduler_self_.get(); }
  Scheduler* GetParentScheduler() const { return scheduler_parent_.get(); }

  uint32_t NewContext();
  void ClearContext(uint32_t id);
  v8::Local<v8::Context> GetContext(uint32_t id);

 private:
  v8::Isolate* isolate_self_{nullptr};
  node::IsolateData* isolate_data_{nullptr};
  uint32_t index_{0};

  std::map<uint32_t, RemoteHandle<v8::Context>> context_map_;

  v8::Isolate* isolate_parent_{nullptr};

  std::unique_ptr<Scheduler> scheduler_self_;
  std::unique_ptr<Scheduler> scheduler_parent_;

  std::unique_ptr<PerIsolateData> per_isolate_data_;
  std::unique_ptr<node::ArrayBufferAllocator> allocator_;

  size_t memory_limit = 128;
};

}  // namespace svm
