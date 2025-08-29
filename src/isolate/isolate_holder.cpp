#include "isolate_holder.h"

#include "../base/logger.h"
#include "../module/context_handle.h"
#include "../utils/utils.h"
#include "../web/local_dom_window.h"
#include "platform_delegate.h"

namespace svm {

IsolateHolder::IsolateHolder(IsolateParams& params)
    : isolate_params_{params}, isolate_par_{params.isolate_par} {
  LOG_INFO("IsolateHolder create.");

  v8::ResourceConstraints rc;
  size_t young_space_in_kb{static_cast<size_t>(
      std::pow(2, std::min(sizeof(void*) >= 8 ? 4.0 : 3.0,
                           isolate_params_.memory_limit / 128.0) +
                      10))};
  size_t old_generation_size_in_mb{isolate_params_.memory_limit};
  rc.set_max_young_generation_size_in_bytes(young_space_in_kb * 1024);
  rc.set_max_old_generation_size_in_bytes(old_generation_size_in_mb * 1024 *
                                          1024);

  auto allocator{node::ArrayBufferAllocator::Create()};
  v8::Isolate::CreateParams create_params{};
  create_params.array_buffer_allocator = allocator.get();

  isolate_sel_ = v8::Isolate::Allocate();
  scheduler_sel_ =
      std::make_unique<UVSchedulerSel>(isolate_sel_, std::move(allocator));
  PlatformDelegate::RegisterIsolate(isolate_sel_, scheduler_sel_->GetLoop());
  v8::Isolate::Initialize(isolate_sel_, create_params);
  scheduler_sel_->StartLoop();
  isolate_sel_->AddNearHeapLimitCallback(
      [](void* data, size_t current_limit, size_t initial_limit) {
        // 增加100MB堆限制
        return current_limit + 100 * 1024 * 1024;
      },
      nullptr);
  per_isolate_data_ =
      std::make_unique<PerIsolateData>(isolate_sel_, scheduler_sel_.get());

  scheduler_par_ = PerIsolateData::From(isolate_par_)->GetScheduler();
}

IsolateHolder::~IsolateHolder() {
  LOG_INFO("Isolate holder delete.");

  context_map_.clear();
  unbound_script_map_.clear();
  per_isolate_data_.reset();
  scheduler_sel_.reset();
}

void IsolateHolder::PostMacroTaskToSel(std::unique_ptr<v8::Task> task) const {
  scheduler_sel_->PostMacroTask(std::move(task));
}
void IsolateHolder::PostMicroTaskToSel(std::unique_ptr<v8::Task> task) const {
  scheduler_sel_->PostMicroTask(std::move(task));
}
uint32_t IsolateHolder::PostTimeoutTaskToSel(std::unique_ptr<v8::Task> task,
                                             uint64_t ms) const {
  return GetTimerManagerSel()->AddTimer(Timer::Type::ktimeout, ms,
                                        std::move(task));
}
uint32_t IsolateHolder::PostIntervalTaskToSel(std::unique_ptr<v8::Task> task,
                                              uint64_t ms) const {
  return GetTimerManagerSel()->AddTimer(Timer::Type::kInterval, ms,
                                        std::move(task));
}

void IsolateHolder::PostInterruptTaskToSel(
    std::unique_ptr<v8::Task> task) const {
  scheduler_sel_->PostInterruptTask(std::move(task));
}
void IsolateHolder::PostMacroTaskToPar(std::unique_ptr<v8::Task> task) const {
  scheduler_par_->PostMacroTask(std::move(task));
}
void IsolateHolder::PostMicroTaskToPar(std::unique_ptr<v8::Task> task) const {
  scheduler_par_->PostMicroTask(std::move(task));
}
void IsolateHolder::PostDelayedTaskToPar(std::unique_ptr<v8::Task> task,
                                         double delay_in_seconds) const {
  scheduler_par_->TaskRunner()->PostDelayedTask(std::move(task),
                                                delay_in_seconds);
}
void IsolateHolder::PostInterruptTaskToPar(
    std::unique_ptr<v8::Task> task) const {
  scheduler_par_->PostInterruptTask(std::move(task));
}

static void DeserializeInternalFieldsCallback(v8::Local<v8::Object> /*holder*/,
                                              int /*index*/,
                                              v8::StartupData /*payload*/,
                                              void* /*data*/) {}
ContextId IsolateHolder::CreateContext() {
  v8::Isolate::Scope isolate_scope{isolate_sel_};
  v8::HandleScope handle_scope{isolate_sel_};

  v8::Local object_template{V8Window::GetWrapperTypeInfo()
                                ->GetV8ClassTemplate(isolate_sel_)
                                .As<v8::FunctionTemplate>()
                                ->InstanceTemplate()};

  v8::Local context{v8::Context::New(isolate_sel_, nullptr, object_template, {},
                                     &DeserializeInternalFieldsCallback)};
  ScriptWrappable::Wrap(
      context->Global(),
      MakeCppGcObject<GC::kSpecified, LocalDOMWindow>(isolate_sel_, this));

  context->AllowCodeGenerationFromStrings(false);

  context_map_.emplace(*context, context);

  return *context;
}

void IsolateHolder::ClearContext(ContextId address) {
  std::lock_guard lock{mutex_context_map_};
  context_map_.erase(address);
}

v8::Local<v8::Context> IsolateHolder::GetContext(ContextId address) {
  const auto it{context_map_.find(address)};
  if (it != context_map_.end()) {
    return it->second.Get(isolate_sel_);
  }
  return {};
}
void IsolateHolder::CreateScript(v8::Local<v8::UnboundScript> unbound_script) {
  unbound_script_map_.emplace(*unbound_script,
                              RemoteHandle{isolate_sel_, unbound_script});
}
v8::Local<v8::UnboundScript> IsolateHolder::GetScript(ScriptId address) {
  const auto it{unbound_script_map_.find(address)};
  if (it != unbound_script_map_.end()) {
    return it->second.Get(isolate_sel_);
  }
  return {};
}
void IsolateHolder::ClearScript(ScriptId address) {
  std::lock_guard lock{mutex_unbound_script_map_};
  unbound_script_map_.erase(address);
}

}  // namespace svm
