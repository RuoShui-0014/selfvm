#include "isolate_handle.h"

#include <cppgc/member.h>
#include <cppgc/visitor.h>

#include "base/logger.h"
#include "isolate/isolate_holder.h"
#include "isolate/task.h"
#include "module/context_handle.h"
#include "module/script_handle.h"
#include "module/session_handle.h"
#include "utils/string.h"
#include "utils/utils.h"

namespace svm {

class CreateContextTask final : public SyncTask<v8::Context*> {
 public:
  explicit CreateContextTask(
      const std::shared_ptr<IsolateHolder>& isolate_holder)
      : isolate_holder_{isolate_holder} {}
  ~CreateContextTask() override = default;

  void Run() override {
    v8::Context* address{isolate_holder_->CreateContext()};
    SetResult(address);
  }

 private:
  std::shared_ptr<IsolateHolder> isolate_holder_;
};
class CreateContextAsyncTask final : public AsyncTask {
 public:
  class Callback final : public v8::Task {
   public:
    explicit Callback(std::unique_ptr<AsyncInfo> info,
                      IsolateHandle* isolate_handle,
                      v8::Context* address)
        : info_{std::move(info)},
          isolate_handle_{isolate_handle},
          address_{address} {}
    ~Callback() override = default;

    void Run() override {
      v8::Isolate* isolate{info_->GetIsolatePar()};
      v8::Local context{info_->context.Get(isolate)};

      ContextHandle* context_handle{
          MakeCppGcObject<GC::kSpecified, ContextHandle>(
              isolate, isolate_handle_.Get(), address_)};

      v8::Local obj{NewInstance<V8ContextHandle>(
          isolate, isolate->GetCurrentContext(), context_handle)};
      info_->resolver.Get(isolate)->Resolve(context, obj);
    }

    std::unique_ptr<AsyncInfo> info_;
    cppgc::Member<IsolateHandle> isolate_handle_;
    v8::Context* const address_;
  };
  explicit CreateContextAsyncTask(std::unique_ptr<AsyncInfo> info,
                                  IsolateHandle* isolate_handle)
      : AsyncTask{std::move(info)}, isolate_handle_{isolate_handle} {}
  ~CreateContextAsyncTask() override = default;

  void Run() override {
    v8::Context* const address{info_->isolate_holder_->CreateContext()};
    info_->PostHandleTaskToPar(std::make_unique<Callback>(
        std::move(info_), isolate_handle_.Get(), address));
  }

 private:
  cppgc::Member<IsolateHandle> isolate_handle_;
};

class CompileScriptTask final : public SyncTask<ScriptId> {
 public:
  CompileScriptTask(const std::shared_ptr<IsolateHolder>& isolate_holder,
                    ContextId context_id,
                    std::string& script,
                    std::string& filename)
      : isolate_holder_{isolate_holder},
        context_id_{context_id},
        script_{std::move(script)},
        filename_{std::move(filename)} {}
  ~CompileScriptTask() override = default;

  void Run() override {
    v8::Isolate* isolate{isolate_holder_->GetIsolateSel()};
    v8::Local context{isolate_holder_->GetContext(context_id_)};
    v8::Context::Scope scope{context};

    v8::ScriptOrigin origin{isolate, toString(isolate, filename_)};
    v8::ScriptCompiler::Source source{toString(isolate, script_), origin};

    v8::Local<v8::UnboundScript> unbound_script{};
    if (v8::ScriptCompiler::CompileUnboundScript(isolate, &source)
            .ToLocal(&unbound_script)) {
      isolate_holder_->CreateScript(unbound_script);
      SetResult(*unbound_script);
    } else {
      SetResult(nullptr);
    }
  }

 private:
  std::shared_ptr<IsolateHolder> isolate_holder_;
  ContextId context_id_;
  std::string script_;
  std::string filename_;
};
class CompileScriptAsyncTask final : public AsyncTask {
 public:
  class Callback final : public v8::Task {
   public:
    Callback(std::unique_ptr<AsyncInfo> info,
             IsolateHandle* isolate_handle,
             v8::UnboundScript* unbound_script)
        : info_{std::move(info)},
          isolate_handle_{isolate_handle},
          unbound_script_{unbound_script} {}
    ~Callback() override = default;

    void Run() override {
      v8::Isolate* isolate{info_->GetIsolatePar()};
      v8::Local context{isolate->GetCurrentContext()};
      ScriptHandle* script_handle{MakeCppGcObject<GC::kSpecified, ScriptHandle>(
          isolate, isolate_handle_, unbound_script_)};
      v8::Local target{
          NewInstance<V8ScriptHandle>(isolate, context, script_handle)};
      info_->resolver.Get(isolate)->Resolve(context, target);
    }

    std::unique_ptr<AsyncInfo> info_;
    cppgc::Member<IsolateHandle> isolate_handle_;
    v8::UnboundScript* unbound_script_;
  };
  CompileScriptAsyncTask(std::unique_ptr<AsyncInfo> info,
                         ContextHandle* context_handle,
                         std::string script,
                         std::string filename)
      : AsyncTask{std::move(info)},
        context_handle_{context_handle},
        script_{std::move(script)},
        filename_{std::move(filename)} {}
  ~CompileScriptAsyncTask() override = default;

  void Run() override {
    v8::Isolate* isolate{info_->GetIsolateSel()};
    v8::Local context{context_handle_->GetContext()};
    v8::Context::Scope scope{context};

    v8::ScriptOrigin origin(isolate, toString(isolate, filename_));
    v8::ScriptCompiler::Source source(toString(isolate, script_), origin);

    v8::Local<v8::UnboundScript> unbound_script{};
    if (v8::ScriptCompiler::CompileUnboundScript(isolate, &source)
            .ToLocal(&unbound_script)) {
      context_handle_->GetIsolateHolder()->CreateScript(unbound_script);
      info_->PostHandleTaskToPar(std::make_unique<Callback>(
          std::move(info_), context_handle_->GetIsolateHandle(),
          *unbound_script));
    } else {
      info_->PostHandleTaskToPar(std::make_unique<Callback>(
          std::move(info_), context_handle_->GetIsolateHandle(), nullptr));
    }
  }

 private:
  cppgc::WeakMember<ContextHandle> context_handle_;
  std::string script_;
  std::string filename_;
};

class IsolateGcTask final : public SyncTask<void> {
 public:
  explicit IsolateGcTask(v8::Isolate* isolate) : isolate_{isolate} {}
  ~IsolateGcTask() override = default;

  void Run() override { isolate_->LowMemoryNotification(); }

 private:
  v8::Isolate* isolate_;
};

IsolateHandle* IsolateHandle::Create(IsolateParams& params) {
  return MakeCppGcObject<GC::kSpecified, IsolateHandle>(params.isolate_par,
                                                        params);
}

IsolateHandle::IsolateHandle(IsolateParams& params)
    : isolate_holder_{std::make_shared<IsolateHolder>(params)} {
  LOG_DEBUG("Isolate handle create.");
}

IsolateHandle::~IsolateHandle() {
  LOG_DEBUG("Isolate handle delete.");
}

std::shared_ptr<IsolateHolder> IsolateHandle::GetIsolateHolder() const {
  return isolate_holder_;
}

ContextHandle* IsolateHandle::GetContextHandle() {
  if (!context_handle_) {
    context_handle_ = CreateContext();
    auto* isolate{isolate_holder_->GetIsolatePar()};
    NewInstance<V8ContextHandle>(isolate, isolate->GetCurrentContext(),
                                 context_handle_.Get());
  }
  return context_handle_.Get();
}

v8::Local<v8::Context> IsolateHandle::GetContext(
    v8::Context* const address) const {
  return isolate_holder_->GetContext(address);
}
v8::Local<v8::UnboundScript> IsolateHandle::GetScript(
    ScriptId const address) const {
  return isolate_holder_->GetScript(address);
}

ContextHandle* IsolateHandle::CreateContext() {
  auto waiter{base::Waiter<ContextId>{}};
  auto task{std::make_unique<CreateContextTask>(isolate_holder_)};
  task->SetWaiter(&waiter);
  isolate_holder_->PostTaskToSel(std::move(task), Scheduler::TaskType::kMacro);
  auto id{waiter.WaitFor()};

  auto* isolate{isolate_holder_->GetIsolatePar()};
  ContextHandle* context_handle{
      MakeCppGcObject<GC::kSpecified, ContextHandle>(isolate, this, id)};
  return context_handle;
}

void IsolateHandle::CreateContextAsync(std::unique_ptr<AsyncInfo> info) {
  auto task{std::make_unique<CreateContextAsyncTask>(std::move(info), this)};
  isolate_holder_->PostTaskToSel(std::move(task), Scheduler::TaskType::kMacro);
}

ScriptHandle* IsolateHandle::CreateScript(std::string script,
                                          std::string filename) {
  auto waiter{base::Waiter<ScriptId>{}};
  auto task{std::make_unique<CompileScriptTask>(
      isolate_holder_, GetContextHandle()->GetContextId(), script, filename)};
  task->SetWaiter(&waiter);
  isolate_holder_->PostTaskToSel(std::move(task), Scheduler::TaskType::kMacro);
  ScriptId address{waiter.WaitFor()};

  if (!address) {
    return nullptr;
  }

  auto* isolate{isolate_holder_->GetIsolatePar()};
  ScriptHandle* script_handle{
      MakeCppGcObject<GC::kSpecified, ScriptHandle>(isolate, this, address)};
  return script_handle;
}
void IsolateHandle::CreateScriptAsync(std::unique_ptr<AsyncInfo> info,
                                      std::string script,
                                      std::string filename) {
  auto task{std::make_unique<CompileScriptAsyncTask>(
      std::move(info), GetContextHandle(), std::move(script),
      std::move(filename))};
  isolate_holder_->PostTaskToSel(std::move(task), Scheduler::TaskType::kMacro);
}

SessionHandle* IsolateHandle::GetInspectorSession() {
  if (!session_handle_) {
    auto* isolate{isolate_holder_->GetIsolatePar()};
    SessionHandle* session_handle{
        MakeCppGcObject<GC::kSpecified, SessionHandle>(isolate, this)};
    NewInstance<V8SessionHandle>(isolate, isolate->GetCurrentContext(),
                                 session_handle);
    session_handle_ = session_handle;
  }
  return session_handle_.Get();
}

void IsolateHandle::IsolateGc() const {
  isolate_holder_->PostTaskToSel(
      std::make_unique<IsolateGcTask>(isolate_holder_->GetIsolateSel()),
      Scheduler::TaskType::kMacro);
}

void IsolateHandle::Release() {
  isolate_holder_.reset();
}

v8::HeapStatistics IsolateHandle::GetHeapStatistics() const {
  v8::Isolate* isolate{isolate_holder_->GetIsolateSel()};
  v8::HeapStatistics heap_statistics{};
  isolate->GetHeapStatistics(&heap_statistics);
  return heap_statistics;
}

void IsolateHandle::Trace(cppgc::Visitor* visitor) const {
  visitor->Trace(context_handle_);
  visitor->Trace(session_handle_);
  ScriptWrappable::Trace(visitor);
}

/*************************************************************************************************/
void ConstructCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  struct Table {
    const char* memory_limit{"memoryLimit"};
  };

  v8::Isolate* isolate{info.GetIsolate()};
  if (!info.IsConstructCall()) {
    isolate->ThrowException(
        v8::Exception::TypeError(toString(isolate, "Illegal constructor")));
    return;
  }

  size_t memory_limit{128};
  if (info.Length() > 0 && info[0]->IsObject()) {
    memory_limit = ReadOption<size_t, 128>(
        info[0], StringTable<Table>::Get().memory_limit);
  }

  IsolateParams params{isolate, memory_limit};
  IsolateHandle* isolate_handle{IsolateHandle::Create(params)};
  ScriptWrappable::Wrap(info.This(), isolate_handle);
}

void ContextAttributeGetCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local receiver{info.This()};
  IsolateHandle* isolate_handle{
      ScriptWrappable::Unwrap<IsolateHandle>(receiver)};
  ContextHandle* context_handle{isolate_handle->GetContextHandle()};
  info.GetReturnValue().Set(context_handle->V8Object(info.GetIsolate()));
}

void CreateContextOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::Local receiver{info.This()};
  IsolateHandle* isolate_handle{
      ScriptWrappable::Unwrap<IsolateHandle>(receiver)};
  ContextHandle* context_handle{isolate_handle->CreateContext()};
  v8::Local target{NewInstance<V8ContextHandle>(
      isolate, isolate->GetCurrentContext(), context_handle)};
  info.GetReturnValue().Set(target);
}

void CreateContextAsyncOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::HandleScope scope{isolate};

  v8::Local receiver{info.This()};
  IsolateHandle* isolate_handle{
      ScriptWrappable::Unwrap<IsolateHandle>(receiver)};
  v8::Local resolver{v8::Promise::Resolver::New(isolate->GetCurrentContext())
                         .ToLocalChecked()};

  auto async_info{std::make_unique<AsyncInfo>(
      isolate_handle->GetIsolateHolder(),
      RemoteHandle{isolate, isolate->GetCurrentContext()},
      RemoteHandle{isolate, resolver})};
  isolate_handle->CreateContextAsync(std::move(async_info));

  info.GetReturnValue().Set(resolver->GetPromise());
}

void CreateScriptOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (info.Length() < 1) {
    return;
  }

  v8::Isolate* isolate{info.GetIsolate()};
  std::string script{}, filename{};
  if (info[0]->IsString()) {
    script = *v8::String::Utf8Value(isolate, info[0]);
    if (script == "") {
      isolate->ThrowException(v8::Exception::TypeError(
          toString(isolate, "The script must be a string and not is empty")));
      return;
    }
  }

  if (info.Length() > 1 && info[1]->IsString()) {
    filename = *v8::String::Utf8Value(isolate, info[1]);
  }

  v8::Local receiver{info.This()};
  IsolateHandle* isolate_handle{
      ScriptWrappable::Unwrap<IsolateHandle>(receiver)};
  ScriptHandle* script_handle{
      isolate_handle->CreateScript(std::move(script), std::move(filename))};
  v8::Local target{NewInstance<V8ScriptHandle>(
      isolate, isolate->GetCurrentContext(), script_handle)};
  if (script_handle) {
    info.GetReturnValue().Set(target);
  } else {
    isolate->ThrowException(v8::Exception::TypeError(
        toString(isolate, "Failed to compile the script.")));
  }
}

void CreateScriptAsyncOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (info.Length() < 1) {
    return;
  }

  v8::Isolate* isolate{info.GetIsolate()};
  std::string script{}, filename{};
  if (info[0]->IsString()) {
    script = *v8::String::Utf8Value(isolate, info[0]);
    if (script == "") {
      isolate->ThrowException(v8::Exception::TypeError(
          toString(isolate, "The script must be a string and not is empty")));
      return;
    }
  }

  if (info.Length() > 1 && info[1]->IsString()) {
    filename = *v8::String::Utf8Value(isolate, info[1]);
  }

  v8::Local receiver{info.This()};
  IsolateHandle* isolate_handle{
      ScriptWrappable::Unwrap<IsolateHandle>(receiver)};
  v8::Local resolver{v8::Promise::Resolver::New(isolate->GetCurrentContext())
                         .ToLocalChecked()};

  auto async_info{std::make_unique<AsyncInfo>(
      isolate_handle->GetIsolateHolder(),
      RemoteHandle{isolate, isolate->GetCurrentContext()},
      RemoteHandle{isolate, resolver})};
  isolate_handle->CreateScriptAsync(std::move(async_info), std::move(script),
                                    std::move(filename));

  info.GetReturnValue().Set(resolver->GetPromise());
}

void GetInspectorSessionOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local receiver{info.This()};
  IsolateHandle* isolate_handle{
      ScriptWrappable::Unwrap<IsolateHandle>(receiver)};
  SessionHandle* session_handle{isolate_handle->GetInspectorSession()};
  info.GetReturnValue().Set(session_handle->V8Object(info.GetIsolate()));
}

void GcOperationCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local receiver{info.This()};
  IsolateHandle* isolate_handle{
      ScriptWrappable::Unwrap<IsolateHandle>(receiver)};
  isolate_handle->IsolateGc();
}

void GetHeapStatisticsOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::Local context{isolate->GetCurrentContext()};

  v8::Local receiver{info.This()};
  IsolateHandle* isolate_handle{
      ScriptWrappable::Unwrap<IsolateHandle>(receiver)};
  v8::HeapStatistics heap_info{isolate_handle->GetHeapStatistics()};
  v8::Local heap_statistics{v8::Object::New(isolate)};
  heap_statistics->Set(
      context, toString("total_heap_size"),
      v8::BigInt::NewFromUnsigned(isolate, heap_info.total_heap_size()));
  heap_statistics->Set(context, toString("total_heap_size_executable"),
                       v8::BigInt::NewFromUnsigned(
                           isolate, heap_info.total_heap_size_executable()));
  heap_statistics->Set(
      context, toString("total_physical_size"),
      v8::BigInt::NewFromUnsigned(isolate, heap_info.total_physical_size()));
  heap_statistics->Set(
      context, toString("total_available_size"),
      v8::BigInt::NewFromUnsigned(isolate, heap_info.total_available_size()));
  heap_statistics->Set(
      context, toString("used_heap_size"),
      v8::BigInt::NewFromUnsigned(isolate, heap_info.used_heap_size()));
  heap_statistics->Set(
      context, toString("heap_size_limit"),
      v8::BigInt::NewFromUnsigned(isolate, heap_info.heap_size_limit()));
  heap_statistics->Set(
      context, toString("malloced_memory"),
      v8::BigInt::NewFromUnsigned(isolate, heap_info.malloced_memory()));
  heap_statistics->Set(
      context, toString("peak_malloced_memory"),
      v8::BigInt::NewFromUnsigned(isolate, heap_info.peak_malloced_memory()));
  heap_statistics->Set(
      context, toString("does_zap_garbage"),
      v8::BigInt::NewFromUnsigned(isolate, heap_info.does_zap_garbage()));
  info.GetReturnValue().Set(heap_statistics);
}

void ReleaseOperationCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local receiver{info.This()};
  IsolateHandle* isolate_handle{
      ScriptWrappable::Unwrap<IsolateHandle>(receiver)};
  isolate_handle->Release();
}

void V8IsolateHandle::InstallInterfaceTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::Template> interface_template) {
  ConstructConfig constructor{"Isolate", 0, ConstructCallback};
  AttributeConfig attrs[]{
      {"context", ContextAttributeGetCallback, nullptr,
       v8::PropertyAttribute::ReadOnly, Dependence::kPrototype},
      {"session", GetInspectorSessionOperationCallback, nullptr,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
  };
  OperationConfig operas[]{
      {"createContext", 0, CreateContextOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
      {"createContextAsync", 0, CreateContextAsyncOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},

      {"createScript", 1, CreateScriptOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
      {"createScriptAsync", 1, CreateScriptAsyncOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},

      {"getHeapStatistics", 0, GetHeapStatisticsOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
      {"gc", 0, GcOperationCallback, v8::PropertyAttribute::DontDelete,
       Dependence::kPrototype},
      // {"release", 0, ReleaseOperationCallback,
      //  v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
  };

  InstallConstructor(isolate, interface_template, constructor);

  v8::Local signature{v8::Local<v8::Signature>::Cast(interface_template)};
  InstallAttributes(isolate, interface_template, signature, attrs);
  InstallOperations(isolate, interface_template, signature, operas);
}

const WrapperTypeInfo V8IsolateHandle::wrapper_type_info_{
    "Isolate", V8IsolateHandle::InstallInterfaceTemplate, nullptr};

}  // namespace svm
