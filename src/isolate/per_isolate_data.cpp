#include "per_isolate_data.h"

#include "scheduler.h"
#include "wrapper_type_info.h"

namespace svm {

PerIsolateData::PerIsolateData(v8::Isolate* isolate, Scheduler* scheduler)
    : isolate_{isolate}, scheduler_{scheduler} {
  if (!isolate) {
    return;
  }

  isolate->SetData(IsolateData::kPerIsolateData, this);
}

PerIsolateData::~PerIsolateData() {
  if (isolate_) {
    isolate_->SetData(IsolateData::kPerIsolateData, nullptr);
  }
}

v8::Local<v8::Template> PerIsolateData::FindV8Template(const void* key) {
  if (const auto result{template_map_.find(key)};
      result != template_map_.end()) {
    return result->second.Get(isolate_);
  }
  return v8::Local<v8::Template>{};
}

void PerIsolateData::AddV8Template(const void* key,
                                   v8::Local<v8::Template> value) {
  template_map_.emplace(key, v8::Eternal<v8::Template>(isolate_, value));
}

void PerIsolateData::Reset(v8::Isolate* isolate) {
  if (!isolate) {
    return;
  }

  isolate_ = isolate;
  isolate->SetData(IsolateData::kPerIsolateData, this);
}

Scheduler* PerIsolateData::GetScheduler() const {
  return scheduler_;
}

}  // namespace svm
