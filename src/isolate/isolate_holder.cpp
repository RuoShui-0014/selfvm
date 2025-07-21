#include "isolate_holder.h"

#include <iostream>

#include "../module/context_handle.h"
#include "../utils/utils.h"
#include "../web/local_dom_window.h"
#include "platform_delegate.h"

namespace svm {

IsolateHolder::IsolateHolder(IsolateParams& params)
    : isolate_par_{params.isolate_par}, isolate_params_(params) {
  v8::ResourceConstraints rc;
  size_t young_space_in_kb = static_cast<size_t>(std::pow(
      2, std::min(sizeof(void*) >= 8 ? 4.0 : 3.0, params.memory_limit / 128.0) +
             10));
  size_t old_generation_size_in_mb = params.memory_limit;
  rc.set_max_young_generation_size_in_bytes(young_space_in_kb * 1024);
  rc.set_max_old_generation_size_in_bytes(old_generation_size_in_mb * 1024 *
                                          1024);

  auto allocator = node::ArrayBufferAllocator::Create();
  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = allocator.get();
  {
    isolate_sel_ = v8::Isolate::Allocate();
    scheduler_sel_ =
        std::make_unique<UVSchedulerSel>(isolate_sel_, std::move(allocator));
    PlatformDelegate::RegisterIsolate(isolate_sel_,
                                      scheduler_sel_->GetUvLoop());
  }
  v8::Isolate::Initialize(isolate_sel_, create_params);
  static_cast<UVSchedulerSel*>(scheduler_sel_.get())->RunTaskLoop();

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

ContextId IsolateHolder::CreateContext() {
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

  context_map_.emplace(*context, context);

  return *context;
}

void IsolateHolder::ClearContext(v8::Context* address) {
  context_map_.erase(address);
}

v8::Local<v8::Context> IsolateHolder::GetContext(v8::Context* address) {
  auto it = context_map_.find(address);
  if (it != context_map_.end()) {
    return it->second.Get(isolate_sel_);
  }
  return {};
}
void IsolateHolder::CreateUnboundScript(
    v8::Local<v8::UnboundScript> unbound_script) {
  unbound_script_map_.emplace(*unbound_script,
                              RemoteHandle{isolate_sel_, unbound_script});
}
v8::Local<v8::UnboundScript> IsolateHolder::GetUnboundScript(
    v8::UnboundScript* address) {
  auto it = unbound_script_map_.find(address);
  if (it != unbound_script_map_.end()) {
    return it->second.Get(isolate_sel_);
  }
  return {};
}

}  // namespace svm
