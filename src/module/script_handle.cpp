#include "script_handle.h"

#include "base/logger.h"
#include "isolate/external_data.h"
#include "isolate/isolate_holder.h"
#include "isolate/task.h"
#include "module/context_handle.h"
#include "module/isolate_handle.h"
#include "utils/utils.h"

namespace svm {

class ScriptRunTask final : public SyncTask<std::pair<uint8_t*, size_t>> {
 public:
  ScriptRunTask(const std::shared_ptr<IsolateHolder>& isolate_holder,
                ContextId context_id,
                ScriptId script_id)
      : isolate_holder_{isolate_holder},
        context_id_{context_id},
        script_id_{script_id} {}
  ~ScriptRunTask() override = default;

  void Run() override {
    v8::Isolate* isolate{isolate_holder_->GetIsolateSel()};
    v8::Local context{isolate_holder_->GetContext(context_id_)};
    v8::Context::Scope scope{context};

    v8::TryCatch try_catch{isolate};
    v8::Local unbound_script{isolate_holder_->GetScript(script_id_)};
    v8::Local script{unbound_script->BindToCurrentContext()};
    v8::Local<v8::Value> result{};
    const bool right{script->Run(context).ToLocal(&result)};
    if (!GetWaiter()) {
      return;
    }

    if (right) {
      ExternalData::SourceData data{isolate, context, result};
      SetResult(ExternalData::SerializerSync(data));
      return;
    }

    if (try_catch.HasCaught()) {
      ExternalData::SourceData data{isolate, context, try_catch.Exception()};
      SetResult(ExternalData::SerializerSync(data));
      try_catch.Reset();
      return;
    }

    SetResult({});
  }

 private:
  std::shared_ptr<IsolateHolder> isolate_holder_;
  const ContextId context_id_;
  const ScriptId script_id_;
};

ScriptHandle::ScriptHandle(IsolateHandle* isolate_handle, ScriptId address)
    : isolate_handle_{isolate_handle},
      isolate_holder_{isolate_handle->GetIsolateHolder()},
      address_{address} {
  LOG_DEBUG("Script handle create.");
}
ScriptHandle::~ScriptHandle() {
  LOG_DEBUG("Script handle delete.");
}

v8::Local<v8::UnboundScript> ScriptHandle::GetScript() const {
  return isolate_holder_->GetScript(address_);
}

std::pair<uint8_t*, size_t> ScriptHandle::Run(
    const ContextHandle* context_handle) {
  auto waiter{base::Waiter<std::pair<uint8_t*, size_t>>{}};
  auto task{std::make_unique<ScriptRunTask>(
      isolate_holder_, context_handle->GetContextId(), address_)};
  task->SetWaiter(&waiter);
  isolate_holder_->PostTaskToSel(std::move(task), Scheduler::TaskType::kMacro);
  return waiter.WaitFor();
}

void ScriptHandle::RunIgnored(const ContextHandle* context_handle) {
  auto task{std::make_unique<ScriptRunTask>(
      isolate_holder_, context_handle->GetContextId(), address_)};
  isolate_holder_->PostTaskToSel(std::move(task), Scheduler::TaskType::kMacro);
}

void ScriptHandle::Release() const {
  isolate_holder_->ClearScript(address_);
}

void ScriptHandle::Trace(cppgc::Visitor* visitor) const {
  visitor->Trace(isolate_handle_);
  ScriptWrappable::Trace(visitor);
}

////////////////////////////////////////////////////////////////////////////////
void RunOperationCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (info.Length() < 1 || !info[0]->IsObject()) {
    return;
  }

  v8::Isolate* isolate{info.GetIsolate()};
  if (!IsInstance<V8ContextHandle>(isolate, info[0])) {
    isolate->ThrowException(v8::Exception::TypeError(
        toString(isolate, "The first argument must be Context.")));
    return;
  }

  v8::HandleScope scope{isolate};
  v8::Local context{isolate->GetCurrentContext()};

  ScriptHandle* script_handle{
      ScriptWrappable::Unwrap<ScriptHandle>(info.This())};
  ContextHandle* context_handle{
      ScriptWrappable::Unwrap<ContextHandle>(info[0].As<v8::Object>())};
  auto buff{script_handle->Run(context_handle)};
  v8::Local result{ExternalData::DeserializerSync(isolate, context, buff)};
  if (!result->IsNativeError()) {
    info.GetReturnValue().Set(result);
  } else {
    isolate->ThrowException(result);
  }
}

void RunIgnoredOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (info.Length() < 1 || !info[0]->IsObject()) {
    return;
  }

  v8::Isolate* isolate{info.GetIsolate()};
  if (!IsInstance<V8ContextHandle>(isolate, info[0])) {
    isolate->ThrowException(v8::Exception::TypeError(
        toString(isolate, "The first argument must be Context.")));
    return;
  }

  v8::HandleScope scope{isolate};

  ScriptHandle* script_handle{
      ScriptWrappable::Unwrap<ScriptHandle>(info.This())};
  const ContextHandle* context_handle{
      ScriptWrappable::Unwrap<ContextHandle>(info[0].As<v8::Object>())};
  script_handle->RunIgnored(context_handle);
}

void V8ScriptHandle::InstallInterfaceTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::Template> interface_template) {
  ConstructConfig constructor{"Script", 0, nullptr};
  InstallConstructor(isolate, interface_template, constructor);

  // AttributeConfig attrs[]{};
  OperationConfig operas[]{
      {"run", 0, RunOperationCallback, v8::PropertyAttribute::DontDelete,
       Dependence::kPrototype},
      {"runIgnored", 0, RunIgnoredOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
  };

  v8::Local signature{v8::Local<v8::Signature>::Cast(interface_template)};
  // InstallAttributes(isolate, interface_template, signature, attrs);
  InstallOperations(isolate, interface_template, signature, operas);
}

const WrapperTypeInfo V8ScriptHandle::wrapper_type_info_{
    "Script", V8ScriptHandle::InstallInterfaceTemplate, nullptr};

}  // namespace svm
