#include "isolate_holder.h"

#include "base/check.h"
#include "base/logger.h"
#include "isolate/platform_delegate.h"
#include "module/context_handle.h"
#include "tool/tools.h"
#include "utils/utils.h"
#include "web/local_dom_window.h"

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

  CHECK(PlatformDelegate::GetNodePlatform(), "");
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
  scheduler_par_->Ref();
}

IsolateHolder::~IsolateHolder() {
  LOG_INFO("Isolate holder delete.");
  Release();
}

void IsolateHolder::PostTaskToSel(std::unique_ptr<v8::Task> task,
                                  const Scheduler::TaskType type) const {
  scheduler_sel_->PostTask(std::move(task), type);
}
uint32_t IsolateHolder::PostDelayedTaskToSel(std::unique_ptr<v8::Task> task,
                                             uint64_t ms,
                                             Timer::Type type) const {
  return scheduler_sel_->PostDelayedTask(std::move(task), ms, type);
}

void IsolateHolder::PostTaskToPar(std::unique_ptr<v8::Task> task,
                                  const Scheduler::TaskType type) const {
  scheduler_par_->PostTask(std::move(task), type);
}

void IsolateHolder::PostDelayedTaskToPar(std::unique_ptr<v8::Task> task,
                                         double delay_in_seconds) const {
  scheduler_par_->TaskRunner()->PostDelayedTask(std::move(task),
                                                delay_in_seconds);
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
  object_template->Set(toString(isolate_sel_, "rsvm"),
                       CreateRsVM(isolate_sel_, true));

  v8::Local context{v8::Context::New(isolate_sel_, nullptr, object_template, {},
                                     &DeserializeInternalFieldsCallback)};
  ScriptWrappable::Wrap(
      context->Global(),
      MakeCppGcObject<GC::kSpecified, LocalDOMWindow>(isolate_sel_, this));

  // context->AllowCodeGenerationFromStrings(false);

  context_map_.emplace(*context, context);

  return *context;
}
v8::Local<v8::Context> IsolateHolder::GetContext(ContextId address) {
  if (const auto it{context_map_.find(address)}; it != context_map_.end()) {
    return it->second.Get(isolate_sel_);
  }
  return {};
}
void IsolateHolder::ClearContext(ContextId address) {
  std::lock_guard lock{mutex_context_map_};
  context_map_.erase(address);
}

void IsolateHolder::CreateScript(v8::Local<v8::UnboundScript> unbound_script) {
  unbound_script_map_.emplace(*unbound_script,
                              RemoteHandle{isolate_sel_, unbound_script});
}

v8::Local<v8::UnboundScript> IsolateHolder::GetScript(ScriptId address) {
  if (const auto it{unbound_script_map_.find(address)};
      it != unbound_script_map_.end()) {
    return it->second.Get(isolate_sel_);
  }
  return {};
}
void IsolateHolder::ClearScript(ScriptId address) {
  std::lock_guard lock{mutex_unbound_script_map_};
  unbound_script_map_.erase(address);
}

void IsolateHolder::Release() {
  scheduler_par_->Unref();

  context_map_.clear();
  unbound_script_map_.clear();

  per_isolate_data_.reset();
}

}  // namespace svm
