#include "isolate_holder.h"

#include <iostream>
#include <mutex>

#include "../lib/thread_pool.h"
#include "../module/isolate_handle.h"
#include "../utils/utils.h"
#include "../web/local_dom_window.h"
#include "platform_delegate.h"

namespace svm {

std::mutex isolate_allocator_mutex{};

IsolateHolder::IsolateHolder(v8::Isolate* isolate_parent,
                             size_t memory_limit_in_mb)
    : isolate_parent_{isolate_parent}, isolate_data_{nullptr} {
  memory_limit = memory_limit_in_mb * 1024 * 1024;
  v8::ResourceConstraints rc;
  size_t young_space_in_kb = static_cast<size_t>(std::pow(
      2, std::min(sizeof(void*) >= 8 ? 4.0 : 3.0, memory_limit_in_mb / 128.0) +
             10));
  size_t old_generation_size_in_mb = memory_limit_in_mb;
  rc.set_max_young_generation_size_in_bytes(young_space_in_kb * 1024);
  rc.set_max_old_generation_size_in_bytes(old_generation_size_in_mb * 1024 *
                                          1024);

  allocator_ = node::ArrayBufferAllocator::Create();
  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = allocator_.get();
  {
    std::lock_guard lock{isolate_allocator_mutex};
    isolate_self_ = v8::Isolate::Allocate();
    scheduler_self_ = std::make_unique<UVScheduler<true>>(isolate_self_);
    PlatformDelegate::RegisterIsolate(isolate_self_,
                                      scheduler_self_->GetUVLoop());
  }
  v8::Isolate::Initialize(isolate_self_, create_params);
  scheduler_self_->RunLoop();

  scheduler_parent_ = std::make_unique<UVScheduler<false>>(isolate_parent_);

  per_isolate_data_ =
      std::move(std::make_unique<PerIsolateData>(isolate_self_));
}

IsolateHolder::~IsolateHolder() {
  context_map_.clear();
  per_isolate_data_.reset();
  scheduler_self_.reset();
  scheduler_parent_.reset();
}

static void DeserializeInternalFieldsCallback(v8::Local<v8::Object> /*holder*/,
                                              int /*index*/,
                                              v8::StartupData /*payload*/,
                                              void* /*data*/) {}

uint32_t IsolateHolder::NewContext() {
  v8::Isolate::Scope isolate_scope(isolate_self_);
  v8::HandleScope handle_scope(isolate_self_);

  v8::Local<v8::ObjectTemplate> object_template =
      V8Window::GetWrapperTypeInfo()
          ->GetV8ClassTemplate(isolate_self_)
          .As<v8::FunctionTemplate>()
          ->InstanceTemplate();

  v8::Local<v8::Context> context =
      v8::Context::New(isolate_self_, nullptr, object_template, {},
                       &DeserializeInternalFieldsCallback);
  ScriptWrappable::Wrap(
      context->Global(),
      MakeCppGcObject<GC::kSpecified, LocalDOMWindow>(isolate_self_));

  context->AllowCodeGenerationFromStrings(false);

  uint32_t id = index_++;
  context_map_.emplace(id, context);

  return id;
}

void IsolateHolder::ClearContext(uint32_t id) {
  context_map_.erase(id);
}

v8::Local<v8::Context> IsolateHolder::GetContext(uint32_t id) {
  auto it = context_map_.find(id);
  if (it != context_map_.end()) {
    return it->second.Get(isolate_self_);
  }
  return {};
}

}  // namespace svm
