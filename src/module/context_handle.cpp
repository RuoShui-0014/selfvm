#include "context_handle.h"

#include <cppgc/visitor.h>

#include "base/logger.h"
#include "isolate/external_data.h"
#include "isolate/isolate_holder.h"
#include "isolate/task.h"
#include "module/isolate_handle.h"
#include "utils/utils.h"

namespace svm {

class EvalTask final : public SyncTask<CopyData> {
 public:
  EvalTask(const std::shared_ptr<IsolateHolder>& isolate_holder,
           ContextId context_id,
           std::string script,
           std::string filename)
      : isolate_holder_{isolate_holder},
        context_id_{context_id},
        script_{std::move(script)},
        filename_{std::move(filename)} {}
  ~EvalTask() override = default;

  void Run() override {
    v8::Isolate* isolate{isolate_holder_->GetIsolateSel()};
    v8::Local context{isolate_holder_->GetContext(context_id_)};
    v8::Context::Scope context_scope{context};

    v8::TryCatch try_catch{isolate};
    v8::Local<v8::Script> script{};
    v8::Local code{toString(isolate, script_)};
    v8::ScriptOrigin scriptOrigin{toString(isolate, filename_)};

    v8::Local<v8::Value> result{};
    bool right{
        v8::Script::Compile(context, code, &scriptOrigin).ToLocal(&script)};
    if (right) {
      right = {script->Run(context).ToLocal(&result)};
    }
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
  ContextId context_id_;
  std::string script_;
  std::string filename_;
};
class EvalAsyncTask final : public AsyncTask {
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
  explicit EvalAsyncTask(std::unique_ptr<AsyncInfo>& info,
                         ContextId context_id,
                         std::string script,
                         std::string filename)
      : AsyncTask{std::move(info)},
        context_id_{context_id},
        script_{std::move(script)},
        filename_{std::move(filename)} {}
  ~EvalAsyncTask() override = default;

  void Run() override {
    v8::Isolate* isolate{info_->GetIsolateSel()};
    auto& isolate_holder{info_->isolate_holder_};
    v8::Local context{isolate_holder->GetContext(context_id_)};
    v8::Context::Scope context_scope{context};

    v8::TryCatch try_catch{isolate};
    v8::Local<v8::Script> script{};
    v8::Local code{toString(isolate, script_)};
    v8::ScriptOrigin scriptOrigin{toString(isolate, filename_)};
    if (v8::Script::Compile(context, code, &scriptOrigin).ToLocal(&script)) {
      v8::MaybeLocal maybe_result{script->Run(context)};
      if (!maybe_result.IsEmpty()) {
        CopyData copy_data{context, maybe_result.ToLocalChecked()};
        info_->PostTaskToPar(
            std::make_unique<Callback>(info_, std::move(copy_data)));
        return;
      }
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
  std::string script_;
  std::string filename_;
};

ContextHandle::ContextHandle(IsolateHandle* isolate_handle,
                             v8::Context* address)
    : isolate_handle_{isolate_handle},
      isolate_holder_{isolate_handle->GetIsolateHolder()},
      context_id_{address} {
  LOG_DEBUG("Context handle create.");
}

ContextHandle::~ContextHandle() {
  LOG_DEBUG("Context handle delete.");

  Release();
}

// 同步任务
CopyData ContextHandle::Eval(std::string script, std::string filename) const {
  auto waiter{base::LazyWaiter<CopyData>{}};
  auto task{std::make_unique<EvalTask>(isolate_holder_.lock(), GetContextId(),
                                       std::move(script), std::move(filename))};
  task->SetWaiter(&waiter);
  isolate_holder_.lock()->PostTaskToSel(std::move(task),
                                        Scheduler::TaskType::kMacro);
  return std::move(waiter.WaitFor());
}
void ContextHandle::EvalIgnored(std::string script,
                                std::string filename) const {
  auto task{std::make_unique<EvalTask>(isolate_holder_.lock(), GetContextId(),
                                       std::move(script), std::move(filename))};
  isolate_holder_.lock()->PostTaskToSel(std::move(task),
                                        Scheduler::TaskType::kMacro);
}

void ContextHandle::EvalAsync(std::unique_ptr<AsyncInfo> info,
                              std::string script,
                              std::string filename) const {
  auto task{std::make_unique<EvalAsyncTask>(
      info, context_id_, std::move(script), std::move(filename))};
  isolate_holder_.lock()->PostTaskToSel(std::move(task),
                                        Scheduler::TaskType::kMacro);
}

void ContextHandle::Release() const {
  if (const auto isolate_holder{isolate_holder_.lock()}) {
    isolate_holder->ClearContext(context_id_);
  }
}

std::shared_ptr<IsolateHolder> ContextHandle::GetIsolateHolder() const {
  return isolate_holder_.lock();
}

v8::Isolate* ContextHandle::GetIsolateSel() const {
  return isolate_holder_.lock()->GetIsolateSel();
}

v8::Isolate* ContextHandle::GetIsolatePar() const {
  return isolate_holder_.lock()->GetIsolatePar();
}

v8::Local<v8::Context> ContextHandle::GetContext() const {
  return isolate_holder_.lock()->GetContext(context_id_);
}

void ContextHandle::Trace(cppgc::Visitor* visitor) const {
  visitor->Trace(isolate_handle_);
  ScriptWrappable::Trace(visitor);
}

///////////////////////////////////////////////////////////////////////////////
void EvalOperationCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (info.Length() < 1 || !info[0]->IsString()) {
    return;
  }

  v8::Isolate* isolate{info.GetIsolate()};
  v8::HandleScope scope{isolate};
  v8::Local context{isolate->GetCurrentContext()};
  v8::Local receiver{info.This()};
  ContextHandle* context_handle{
      ScriptWrappable::Unwrap<ContextHandle>(receiver)};

  std::string script{*v8::String::Utf8Value{isolate, info[0]}};
  std::string filename{""};
  if (info.Length() > 1) {
    filename = *v8::String::Utf8Value(isolate, info[1]);
  }

  auto copy_data{context_handle->Eval(std::move(script), std::move(filename))};
  if (!copy_data.IsEmpty()) {
    v8::Local result{copy_data.Deserializer(context)};
    if (!result->IsNativeError()) {
      info.GetReturnValue().Set(result);
    } else {
      isolate->ThrowException(result);
    }
  } else {
  }
}

void EvalIgnoredOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (info.Length() < 1 || !info[0]->IsString()) {
    return;
  }

  v8::Isolate* isolate{info.GetIsolate()};
  v8::HandleScope scope{isolate};
  v8::Local receiver{info.This()};
  ContextHandle* context_handle{
      ScriptWrappable::Unwrap<ContextHandle>(receiver)};

  std::string script{*v8::String::Utf8Value(isolate, info[0])};
  std::string filename{""};
  if (info.Length() > 1) {
    filename = *v8::String::Utf8Value(isolate, info[1]);
  }

  context_handle->EvalIgnored(std::move(script), std::move(filename));
}

void EvalAsyncOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  if (info.Length() < 1 || !info[0]->IsString()) {
    isolate->ThrowException(v8::Exception::TypeError(
        toString(isolate, "The first argument must be a string.")));
    return;
  }

  v8::HandleScope scope{isolate};

  v8::Local receiver{info.This()};
  ContextHandle* context_handle{
      ScriptWrappable::Unwrap<ContextHandle>(receiver)};
  v8::Local resolver{v8::Promise::Resolver::New(isolate->GetCurrentContext())
                         .ToLocalChecked()};
  auto async_info{std::make_unique<AsyncInfo>(
      context_handle->GetIsolateHolder(),
      RemoteHandle(isolate, isolate->GetCurrentContext()),
      RemoteHandle(isolate, resolver))};
  std::string script{*v8::String::Utf8Value(isolate, info[0])};
  std::string filename{};
  if (info.Length() > 1) {
    filename = *v8::String::Utf8Value(isolate, info[1]);
  }
  context_handle->EvalAsync(std::move(async_info), std::move(script),
                            std::move(filename));

  info.GetReturnValue().Set(resolver);
}

void V8ContextHandle::InstallInterfaceTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::Template> interface_template) {
  ConstructConfig constructor{"Context", 0, nullptr};
  OperationConfig operas[]{
      {"eval", 1, EvalOperationCallback, v8::PropertyAttribute::DontDelete,
       Dependence::kPrototype},
      {"evalAsync", 1, EvalAsyncOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
      {"evalIgnored", 1, EvalIgnoredOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
  };

  InstallConstructor(isolate, interface_template, constructor);

  v8::Local signature{v8::Local<v8::Signature>::Cast(interface_template)};
  // InstallAttributes(isolate, interface_template, signature, attrs);
  InstallOperations(isolate, interface_template, signature, operas);
}

const WrapperTypeInfo V8ContextHandle::wrapper_type_info_{
    "Context", V8ContextHandle::InstallInterfaceTemplate, nullptr};

}  // namespace svm
