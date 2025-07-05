#include "isolate_holder.h"

#include <mutex>

#include "../lib/thread_pool.h"
#include "platform_delegate.h"

namespace svm {

std::mutex isolate_allocator_mutex{};

IsolateHolder::IsolateHolder(v8::Isolate* parent_isolate,
                             size_t memory_limit_in_mb)
    : parent_isolate_{parent_isolate},
      scheduler_{std::make_shared<Scheduler>()} {
  base::ThreadPool::Get()->PostTask(&Scheduler::Entry, scheduler_.get());

  memory_limit = memory_limit_in_mb * 1024 * 1024;
  v8::ResourceConstraints rc;
  size_t young_space_in_kb = static_cast<size_t>(std::pow(
      2, std::min(sizeof(void*) >= 8 ? 4.0 : 3.0, memory_limit_in_mb / 128.0) +
             10));
  size_t old_generation_size_in_mb = memory_limit_in_mb;
  rc.set_max_young_generation_size_in_bytes(young_space_in_kb * 1024);
  rc.set_max_old_generation_size_in_bytes(old_generation_size_in_mb * 1024 *
                                          1024);

  allocator_ = std::unique_ptr<v8::ArrayBuffer::Allocator>(
      v8::ArrayBuffer::Allocator::NewDefaultAllocator());
  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = allocator_.get();
  {
    std::lock_guard lock{isolate_allocator_mutex};
    self_isolate_ = v8::Isolate::Allocate();
    PlatformDelegate::RegisterIsolate(self_isolate_, scheduler_.get());
  }
  v8::Isolate::Initialize(self_isolate_, create_params);

  per_isolate_data_ =
      std::move(std::make_unique<PerIsolateData>(self_isolate_));
}

IsolateHolder::~IsolateHolder() {
  per_isolate_data_.reset();
  {
    std::lock_guard lock{isolate_allocator_mutex};
    self_isolate_->Dispose();
    PlatformDelegate::UnregisterIsolate(self_isolate_);
  }
}

static void DeserializeInternalFieldsCallback(v8::Local<v8::Object> /*holder*/,
                                              int /*index*/,
                                              v8::StartupData /*payload*/,
                                              void* /*data*/) {}

v8::Local<v8::Context> IsolateHolder::NewContext() {
  v8::Isolate::Scope isolate_scope(self_isolate_);
  v8::HandleScope handle_scope(self_isolate_);
  v8::Local<v8::Context> context = v8::Context::New(
      self_isolate_, nullptr, {}, {}, &DeserializeInternalFieldsCallback);
  context->AllowCodeGenerationFromStrings(false);
  // TODO (but I'm not going to do it): This causes a DCHECK failure in debug
  // builds. Tested nodejs v14.17.3 & v16.5.1.
  // context->SetErrorMessageForCodeGenerationFromStrings(StringTable::Get().codeGenerationError);
  return context;
}

}  // namespace svm
