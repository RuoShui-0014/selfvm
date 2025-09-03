#include "script_handle.h"

#include "base/logger.h"
#include "isolate/external_data.h"
#include "isolate/isolate_holder.h"
#include "isolate/task.h"
#include "module/context_handle.h"
#include "module/isolate_handle.h"
#include "utils/utils.h"

namespace svm {

class ScriptRunTask final : public SyncTask<CopyData> {
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
      CopyData copy_data{context, result};
      SetResult(std::move(copy_data));
      return;
    }

    if (try_catch.HasCaught()) {
      CopyData copy_data{context, try_catch.Exception()};
      SetResult(std::move(copy_data));
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
class ScriptRunAsyncTask final : public AsyncTask {
 public:
  class Callback : public v8::Task {
   public:
    explicit Callback(std::unique_ptr<AsyncInfo>& info, CopyData copy_data)
        : info_{std::move(info)}, copy_data_{std::move(copy_data)} {}
    ~Callback() override = default;

    void Run() override {
      v8::Isolate* isolate{info_->isolate_holder_->GetIsolatePar()};
      v8::Local context{info_->context.Get(isolate)};
      v8::Context::Scope context_scope{context};

      v8::Local<v8::Value> result{}, error{};
      {
        v8::TryCatch try_catch{isolate};
        result = copy_data_.Deserializer(context);
        if (!result.IsEmpty()) {
          if (result->IsNativeError()) {
            error = result;
          }
        } else {
          error = try_catch.Exception();
          try_catch.Reset();
        }
      }

      if (error.IsEmpty()) {
        info_->resolver.Get(isolate)->Resolve(context, result);
      } else {
        info_->resolver.Get(isolate)->Resolve(context, error);
      }
    }

   private:
    std::unique_ptr<AsyncInfo> info_;
    CopyData copy_data_;
  };
  explicit ScriptRunAsyncTask(std::unique_ptr<AsyncInfo>& info,
                              ContextId context_id,
                              ScriptId script_id)
      : AsyncTask{std::move(info)},
        context_id_{context_id},
        script_id_{script_id} {}
  ~ScriptRunAsyncTask() override = default;

  void Run() override {
    v8::Isolate* isolate{info_->GetIsolateSel()};
    auto& isolate_holder{info_->isolate_holder_};
    v8::Local context{isolate_holder->GetContext(context_id_)};
    v8::Context::Scope context_scope{context};

    v8::TryCatch try_catch{isolate};
    v8::Local unbound_script{isolate_holder->GetScript(script_id_)};
    v8::Local script{unbound_script->BindToCurrentContext()};
    v8::Local<v8::Value> result{};
    if (const bool right{script->Run(context).ToLocal(&result)}) {
      CopyData copy_data{context, result};
      info_->PostTaskToPar(
          std::make_unique<Callback>(info_, std::move(copy_data)));
      return;
    }

    if (try_catch.HasCaught()) {
      CopyData copy_data{context, try_catch.Exception()};
      info_->PostTaskToPar(
          std::make_unique<Callback>(info_, std::move(copy_data)));
      try_catch.Reset();
    }
  }

 private:
  const ContextId context_id_;
  const ScriptId script_id_;
};

ScriptHandle::ScriptHandle(IsolateHandle* isolate_handle, ScriptId address)
    : isolate_handle_{isolate_handle},
      isolate_holder_{isolate_handle->GetIsolateHolder()},
      script_id_{address} {
  LOG_DEBUG("Script handle create.");
}
ScriptHandle::~ScriptHandle() {
  LOG_DEBUG("Script handle delete.");
  Release();
}

std::shared_ptr<IsolateHolder> ScriptHandle::GetIsolateHolder() const {
  return isolate_holder_.lock();
}

v8::Local<v8::UnboundScript> ScriptHandle::GetScript() const {
  return isolate_holder_.lock()->GetScript(script_id_);
}

CopyData ScriptHandle::Run(const ContextHandle* context_handle) const {
  auto waiter{base::LazyWaiter<CopyData>{}};
  auto task{std::make_unique<ScriptRunTask>(
      isolate_holder_.lock(), context_handle->GetContextId(), script_id_)};
  task->SetWaiter(&waiter);
  isolate_holder_.lock()->PostTaskToSel(std::move(task),
                                        Scheduler::TaskType::kMacro);
  return std::move(waiter.WaitFor());
}
void ScriptHandle::RunAsync(std::unique_ptr<AsyncInfo> info,
                            ContextId context_id) const {
  auto task{std::make_unique<ScriptRunAsyncTask>(info, context_id, script_id_)};
  isolate_holder_.lock()->PostTaskToSel(std::move(task),
                                        Scheduler::TaskType::kMacro);
}

void ScriptHandle::RunIgnored(const ContextHandle* context_handle) const {
  auto task{std::make_unique<ScriptRunTask>(
      isolate_holder_.lock(), context_handle->GetContextId(), script_id_)};
  isolate_holder_.lock()->PostTaskToSel(std::move(task),
                                        Scheduler::TaskType::kMacro);
}

void ScriptHandle::Release() const {
  if (const auto isolate_holder{isolate_holder_.lock()}) {
    isolate_holder->ClearScript(script_id_);
  }
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
  auto copy_data{script_handle->Run(context_handle)};
  v8::Local result{copy_data.Deserializer(context)};
  if (!result->IsNativeError()) {
    info.GetReturnValue().Set(result);
  } else {
    isolate->ThrowException(result);
  }
}

void RunAsyncOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  if (info.Length() < 1 || !info[0]->IsObject()) {
    isolate->ThrowException(v8::Exception::TypeError(
        toString(isolate, "The first argument must be Context.")));
    return;
  }

  v8::HandleScope scope{isolate};

  v8::Local receiver{info.This()};
  ScriptHandle* script_handle{ScriptWrappable::Unwrap<ScriptHandle>(receiver)};
  ContextHandle* context_handle{
      ScriptWrappable::Unwrap<ContextHandle>(info[0].As<v8::Object>())};
  v8::Local resolver{v8::Promise::Resolver::New(isolate->GetCurrentContext())
                         .ToLocalChecked()};
  auto async_info{std::make_unique<AsyncInfo>(
      script_handle->GetIsolateHolder(),
      RemoteHandle(isolate, isolate->GetCurrentContext()),
      RemoteHandle(isolate, resolver))};
  script_handle->RunAsync(std::move(async_info),
                          context_handle->GetContextId());

  info.GetReturnValue().Set(resolver);
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
      {"run", 1, RunOperationCallback, v8::PropertyAttribute::DontDelete,
       Dependence::kPrototype},
      {"runAsync", 1, RunAsyncOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
      {"runIgnored", 1, RunIgnoredOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
  };

  v8::Local signature{v8::Local<v8::Signature>::Cast(interface_template)};
  // InstallAttributes(isolate, interface_template, signature, attrs);
  InstallOperations(isolate, interface_template, signature, operas);
}

const WrapperTypeInfo V8ScriptHandle::wrapper_type_info_{
    "Script", V8ScriptHandle::InstallInterfaceTemplate, nullptr};

}  // namespace svm
