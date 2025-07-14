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
    : isolate_par_{isolate_parent} {
  memory_limit = memory_limit_in_mb * 1024 * 1024;
  v8::ResourceConstraints rc;
  size_t young_space_in_kb = static_cast<size_t>(std::pow(
      2, std::min(sizeof(void*) >= 8 ? 4.0 : 3.0, memory_limit_in_mb / 128.0) +
             10));
  size_t old_generation_size_in_mb = memory_limit_in_mb;
  rc.set_max_young_generation_size_in_bytes(young_space_in_kb * 1024);
  rc.set_max_old_generation_size_in_bytes(old_generation_size_in_mb * 1024 *
                                          1024);

  auto allocator = node::ArrayBufferAllocator::Create();
  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = allocator.get();
  {
    std::lock_guard lock{isolate_allocator_mutex};
    isolate_sel_ = v8::Isolate::Allocate();
    scheduler_sel_ =
        std::make_unique<UVSchedulerSel>(isolate_sel_, std::move(allocator));
    PlatformDelegate::RegisterIsolate(isolate_sel_,
                                      scheduler_sel_->GetUvLoop());
  }
  v8::Isolate::Initialize(isolate_sel_, create_params);
  reinterpret_cast<UVSchedulerSel*>(scheduler_sel_.get())->RunTaskLoop();

  scheduler_par_ = std::make_unique<UVSchedulerPar>(isolate_par_);

  per_isolate_data_ = std::move(std::make_unique<PerIsolateData>(isolate_sel_));
}

IsolateHolder::~IsolateHolder() {
  context_map_.clear();
  per_isolate_data_.reset();
  scheduler_sel_.reset();
  scheduler_par_.reset();
}

static void DeserializeInternalFieldsCallback(v8::Local<v8::Object> /*holder*/,
                                              int /*index*/,
                                              v8::StartupData /*payload*/,
                                              void* /*data*/) {}

void IsolateHolder::PostTaskToSel(std::unique_ptr<v8::Task> task) {
  scheduler_sel_->TaskRunner()->PostTask(std::move(task));
}
void IsolateHolder::PostTaskToPar(std::unique_ptr<v8::Task> task) {
  scheduler_par_->TaskRunner()->PostTask(std::move(task));
}
void IsolateHolder::PostDelayedTaskToSel(std::unique_ptr<v8::Task> task,
                                         double delay_in_seconds) {
  scheduler_sel_->TaskRunner()->PostDelayedTask(std::move(task),
                                                delay_in_seconds);
}
void IsolateHolder::PostDelayedTaskToPar(std::unique_ptr<v8::Task> task,
                                         double delay_in_seconds) {
  scheduler_par_->TaskRunner()->PostDelayedTask(std::move(task),
                                                delay_in_seconds);
}
void IsolateHolder::PostInspectorTask(std::unique_ptr<v8::Task> task) {
  scheduler_sel_->PostInspectorTask(std::move(task));
}

uint32_t IsolateHolder::NewContext() {
  v8::Isolate::Scope isolate_scope(isolate_sel_);
  v8::HandleScope handle_scope(isolate_sel_);

  v8::Local<v8::ObjectTemplate> object_template =
      V8Window::GetWrapperTypeInfo()
          ->GetV8ClassTemplate(isolate_sel_)
          .As<v8::FunctionTemplate>()
          ->InstanceTemplate();

  v8::Local<v8::Context> context =
      v8::Context::New(isolate_sel_, nullptr, object_template, {},
                       &DeserializeInternalFieldsCallback);
  ScriptWrappable::Wrap(
      context->Global(),
      MakeCppGcObject<GC::kSpecified, LocalDOMWindow>(isolate_sel_));

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
    return it->second.Get(isolate_sel_);
  }
  return {};
}

}  // namespace svm
