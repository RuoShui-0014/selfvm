#include "isolate_handle.h"

#include <cppgc/member.h>
#include <cppgc/visitor.h>

#include <iostream>

#include "../isolate/isolate_holder.h"
#include "../isolate/task.h"
#include "../utils/utils.h"
#include "context_handle.h"
#include "script_handle.h"
#include "session_handle.h"
#include "string.h"

namespace svm {

class CreateContextTask : public SyncTask<v8::Context*> {
 public:
  explicit CreateContextTask(IsolateHandle* isolate_handle)
      : isolate_handle_(isolate_handle) {}
  ~CreateContextTask() override = default;

  void Run() override {
    v8::Context* address = isolate_handle_->GetIsolateHolder()->CreateContext();
    SetResult(address);
  }

 private:
  cppgc::Member<IsolateHandle> isolate_handle_;
};
class CreateContextAsyncTask final : public AsyncTask {
 public:
  class Callback : public v8::Task {
   public:
    explicit Callback(std::unique_ptr<AsyncInfo> info, v8::Context* address)
        : info_(std::move(info)), address_(address) {}
    ~Callback() override = default;

    void Run() override {
      v8::Isolate* isolate = info_->isolate;
      v8::Local<v8::Context> context = info_->context.Get(isolate);
      v8::HandleScope source_scope(isolate);

      ContextHandle* context_handle =
          MakeCppGcObject<GC::kSpecified, ContextHandle>(
              isolate, info_->isolate_handle, address_);
      v8::Local<v8::FunctionTemplate> isolate_handle_template =
          V8ContextHandle::GetWrapperTypeInfo()
              ->GetV8ClassTemplate(isolate)
              .As<v8::FunctionTemplate>();
      v8::Local<v8::Object> obj =
          isolate_handle_template->InstanceTemplate()
              ->NewInstance(isolate->GetCurrentContext())
              .ToLocalChecked();
      ScriptWrappable::Wrap(obj, context_handle);
      info_->resolver.Get(isolate)->Resolve(context, obj);
    }

    std::unique_ptr<AsyncInfo> info_;
    v8::Context* const address_;
  };
  explicit CreateContextAsyncTask(std::unique_ptr<AsyncInfo> info)
      : AsyncTask(std::move(info)) {}
  ~CreateContextAsyncTask() override = default;

  void Run() override {
    v8::Context* const address =
        info_->isolate_handle->GetIsolateHolder()->CreateContext();
    info_->isolate_handle->PostHandleTaskToPar(
        std::make_unique<Callback>(std::move(info_), address));
  }
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
    : isolate_holder_(std::make_shared<IsolateHolder>(params)) {}

IsolateHandle::~IsolateHandle() {
  if (session_handle_) {
    session_handle_->Release();
  }

#ifdef DEBUG
  std::cout << "~IsolateHandle()" << std::endl;
#endif
}

std::shared_ptr<IsolateHolder> IsolateHandle::GetIsolateHolder() const {
  return isolate_holder_;
}

v8::Isolate* IsolateHandle::GetIsolateSel() const {
  return isolate_holder_->GetIsolateSel();
}

v8::Isolate* IsolateHandle::GetIsolatePar() const {
  return isolate_holder_->GetIsolatePar();
}

ContextHandle* IsolateHandle::GetContextHandle() {
  if (!context_handle_) {
    context_handle_ = CreateContext();
  }
  return context_handle_.Get();
}

v8::Local<v8::Context> IsolateHandle::GetContext(
    v8::Context* const address) const {
  return isolate_holder_->GetContext(address);
}
v8::Local<v8::UnboundScript> IsolateHandle::GetScript(
    ScriptId const address) const {
  return isolate_holder_->GetUnboundScript(address);
}

Scheduler* IsolateHandle::GetSchedulerSel() const {
  return isolate_holder_->GetSchedulerSel();
}

Scheduler* IsolateHandle::GetSchedulerPar() const {
  return isolate_holder_->GetSchedulerPar();
}

void IsolateHandle::PostTaskToSel(std::unique_ptr<v8::Task> task) const {
  isolate_holder_->PostTaskToSel(std::move(task));
}
void IsolateHandle::PostHandleTaskToSel(std::unique_ptr<v8::Task> task) const {
  isolate_holder_->PostHandleTaskToSel(std::move(task));
}
void IsolateHandle::PostHandleTaskToPar(std::unique_ptr<v8::Task> task) const {
  isolate_holder_->PostHandleTaskToPar(std::move(task));
}

void IsolateHandle::PostTaskToPar(std::unique_ptr<v8::Task> task) const {
  isolate_holder_->PostTaskToPar(std::move(task));
}
void IsolateHandle::PostInterruptTask(std::unique_ptr<v8::Task> task) const {
  isolate_holder_->PostInterruptTask(std::move(task));
}
void IsolateHandle::AddDebugContext(ContextHandle* context) const {
  session_handle_->AddContext(context);
}

ContextHandle* IsolateHandle::CreateContext() {
  auto task = std::make_unique<CreateContextTask>(this);
  auto future = task->GetFuture();
  PostTaskToSel(std::move(task));
  auto id = future.get();

  v8::Isolate* isolate = GetIsolatePar();
  ContextHandle* context_handle =
      MakeCppGcObject<GC::kSpecified, ContextHandle>(isolate, this, id);
  NewInstance<V8ContextHandle>(isolate, isolate->GetCurrentContext(),
                               context_handle);
  return context_handle;
}

void IsolateHandle::CreateContextAsync(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    v8::Local<v8::Promise::Resolver> resolver) {
  auto async_info =
      std::make_unique<AsyncInfo>(this, isolate, RemoteHandle(isolate, context),
                                  RemoteHandle(isolate, resolver));
  auto task = std::make_unique<CreateContextAsyncTask>(std::move(async_info));
  PostTaskToSel(std::move(task));
}
ScriptHandle* IsolateHandle::CreateScript(std::string script,
                                          std::string filename) {
  return ScriptHandle::Create(this, std::move(script), std::move(filename));
}

SessionHandle* IsolateHandle::GetInspectorSession() {
  if (!session_handle_) {
    v8::Isolate* isolate = GetIsolatePar();
    SessionHandle* session_handle =
        MakeCppGcObject<GC::kSpecified, SessionHandle>(isolate, this);
    NewInstance<V8SessionHandle>(isolate, isolate->GetCurrentContext(),
                                 session_handle);
    session_handle_ = session_handle;
  }
  return session_handle_.Get();
}

void IsolateHandle::IsolateGc() const {
  PostTaskToSel(
      std::make_unique<IsolateGcTask>(isolate_holder_->GetIsolateSel()));
}

void IsolateHandle::Release() {
  isolate_holder_.reset();
}

v8::HeapStatistics IsolateHandle::GetHeapStatistics() const {
  v8::Isolate* isolate = isolate_holder_->GetIsolateSel();
  v8::HeapStatistics heap_statistics;
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

  if (!info.IsConstructCall()) {
    info.GetIsolate()->ThrowException(v8::Exception::TypeError(
        toString(info.GetIsolate(), "Illegal constructor")));
    return;
  }

  v8::Isolate* isolate = info.GetIsolate();
  size_t memory_limit{128};
  if (info.Length() > 0 && info[0]->IsObject()) {
    memory_limit = ReadOption<size_t, 128>(
        info[0], StringTable<Table>::Get().memory_limit);
  }

  IsolateParams params{isolate, memory_limit};
  IsolateHandle* isolate_handle = IsolateHandle::Create(params);
  ScriptWrappable::Wrap(info.This(), isolate_handle);
}

void ContextAttributeGetCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> receiver = info.This();
  IsolateHandle* isolate_handle =
      ScriptWrappable::Unwrap<IsolateHandle>(receiver);
  ContextHandle* context_handle = isolate_handle->GetContextHandle();
  info.GetReturnValue().Set(context_handle->V8Object(info.GetIsolate()));
}

void CreateContextOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> receiver = info.This();
  IsolateHandle* isolate_handle =
      ScriptWrappable::Unwrap<IsolateHandle>(receiver);
  ContextHandle* context_handle = isolate_handle->CreateContext();
  info.GetReturnValue().Set(context_handle->V8Object(info.GetIsolate()));
}

void CreateContextAsyncOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::HandleScope scope(isolate);

  v8::Local<v8::Object> receiver = info.This();
  IsolateHandle* isolate_handle =
      ScriptWrappable::Unwrap<IsolateHandle>(receiver);
  v8::Local<v8::Promise::Resolver> resolver =
      v8::Promise::Resolver::New(isolate->GetCurrentContext()).ToLocalChecked();
  isolate_handle->CreateContextAsync(isolate, isolate->GetCurrentContext(),
                                     resolver);

  info.GetReturnValue().Set(resolver->GetPromise());
}

void CreateScriptOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (info.Length() < 1) {
    return;
  }

  v8::Isolate* isolate = info.GetIsolate();
  std::string script, filename;
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

  v8::Local<v8::Object> receiver = info.This();
  IsolateHandle* isolate_handle =
      ScriptWrappable::Unwrap<IsolateHandle>(receiver);
  ScriptHandle* script_handle =
      isolate_handle->CreateScript(std::move(script), std::move(filename));
  if (script_handle) {
    info.GetReturnValue().Set(script_handle->V8Object(info.GetIsolate()));
  } else {
    isolate->ThrowException(v8::Exception::TypeError(
        toString(isolate, "Failed to compile the script.")));
  }
}

void GetInspectorSessionOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> receiver = info.This();
  IsolateHandle* isolate_handle =
      ScriptWrappable::Unwrap<IsolateHandle>(receiver);
  SessionHandle* session_handle = isolate_handle->GetInspectorSession();
  info.GetReturnValue().Set(session_handle->V8Object(info.GetIsolate()));
}

void GcOperationCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> receiver = info.This();
  IsolateHandle* isolate_handle =
      ScriptWrappable::Unwrap<IsolateHandle>(receiver);
  isolate_handle->IsolateGc();
}

void GetHeapStatisticsOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::Local<v8::Object> receiver = info.This();
  IsolateHandle* isolate_handle =
      ScriptWrappable::Unwrap<IsolateHandle>(receiver);
  v8::HeapStatistics heap_info = isolate_handle->GetHeapStatistics();
  v8::Local<v8::Object> heap_statistics = v8::Object::New(isolate);
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
  v8::Local<v8::Object> receiver = info.This();
  IsolateHandle* isolate_handle =
      ScriptWrappable::Unwrap<IsolateHandle>(receiver);
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

      {"createScript", 0, CreateScriptOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},

      {"getHeapStatistics", 0, GetHeapStatisticsOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
      {"gc", 0, GcOperationCallback, v8::PropertyAttribute::DontDelete,
       Dependence::kPrototype},
      // {"release", 0, ReleaseOperationCallback,
      //  v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
  };

  InstallConstructor(isolate, interface_template, constructor);

  v8::Local<v8::Signature> signature =
      v8::Local<v8::Signature>::Cast(interface_template);
  InstallAttributes(isolate, interface_template, signature, attrs);
  InstallOperations(isolate, interface_template, signature, operas);
}

const WrapperTypeInfo V8IsolateHandle::wrapper_type_info_{
    "Isolate", V8IsolateHandle::InstallInterfaceTemplate, nullptr};

}  // namespace svm
