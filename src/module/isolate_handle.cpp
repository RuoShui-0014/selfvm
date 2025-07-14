#include "isolate_handle.h"

#include <cppgc/member.h>
#include <cppgc/visitor.h>

#include "../isolate/isolate_holder.h"
#include "../isolate/task.h"
#include "../utils/utils.h"
#include "context_handle.h"
#include "session_handle.h"

namespace svm {

IsolateHandle* IsolateHandle::Create(v8::Isolate* isolate_parent) {
  size_t memory_limit = 128;

  auto isolate_holder =
      std::make_unique<IsolateHolder>(isolate_parent, memory_limit);

  return MakeCppGcObject<GC::kSpecified, IsolateHandle>(
      isolate_parent, std::move(isolate_holder));
}

IsolateHandle::IsolateHandle(std::unique_ptr<IsolateHolder> isolate_holder)
    : isolate_holder_(std::move(isolate_holder)) {}

IsolateHandle::~IsolateHandle() {
  default_context_.Clear();
}

v8::Isolate* IsolateHandle::GetIsolateSel() const {
  return isolate_holder_->GetIsolateSel();
}

v8::Isolate* IsolateHandle::GetIsolatePar() const {
  return isolate_holder_->GetIsolatePar();
}

ContextHandle* IsolateHandle::GetContextHandle() {
  if (!default_context_) {
    default_context_ = ContextHandle::Create(this);
  }
  return default_context_.Get();
}

v8::Local<v8::Context> IsolateHandle::GetContext(uint32_t id) {
  return isolate_holder_->GetContext(id);
}

Scheduler* IsolateHandle::GetSchedulerSel() {
  return isolate_holder_->GetSchedulerSel();
}

Scheduler* IsolateHandle::GetSchedulerPar() {
  return isolate_holder_->GetSchedulerPar();
}

void IsolateHandle::PostTaskToSel(std::unique_ptr<v8::Task> task) {
  isolate_holder_->PostTaskToSel(std::move(task));
}

void IsolateHandle::PostTaskToPar(std::unique_ptr<v8::Task> task) {
  isolate_holder_->PostTaskToPar(std::move(task));
}
void IsolateHandle::PostInspectorTask(std::unique_ptr<v8::Task> task) {
  isolate_holder_->PostInspectorTask(std::move(task));
}

ContextHandle* IsolateHandle::CreateContext() {
  return ContextHandle::Create(this);
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

SessionHandle* IsolateHandle::CreateInspectorSession() {
  return MakeCppGcObject<GC::kCurrent, SessionHandle>(this);
}

void IsolateHandle::IsolateGc() {
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
  visitor->Trace(default_context_);
  ScriptWrappable::Trace(visitor);
}

/*************************************************************************************************/

void ConstructCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (info.IsConstructCall()) {
    ScriptWrappable::Wrap(info.This(),
                          IsolateHandle::Create(info.GetIsolate()));
    return;
  }

  info.GetIsolate()->ThrowException(v8::Exception::TypeError(
      toString(info.GetIsolate(), "Illegal constructor")));
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

void CreateInspectorSessionOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> receiver = info.This();
  IsolateHandle* isolate_handle =
      ScriptWrappable::Unwrap<IsolateHandle>(receiver);
  SessionHandle* session_handle = isolate_handle->CreateInspectorSession();

  v8::Local<v8::FunctionTemplate> session_handle_template =
      V8SessionHandle::GetWrapperTypeInfo()
          ->GetV8ClassTemplate(info.GetIsolate())
          .As<v8::FunctionTemplate>();
  v8::Local<v8::Object> obj =
      session_handle_template->InstanceTemplate()
          ->NewInstance(info.GetIsolate()->GetCurrentContext())
          .ToLocalChecked();
  ScriptWrappable::Wrap(obj, session_handle);
  info.GetReturnValue().Set(obj);
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
      v8::Integer::NewFromUnsigned(isolate, heap_info.total_heap_size()));
  heap_statistics->Set(context, toString("total_heap_size_executable"),
                       v8::Integer::NewFromUnsigned(
                           isolate, heap_info.total_heap_size_executable()));
  heap_statistics->Set(
      context, toString("total_physical_size"),
      v8::Integer::NewFromUnsigned(isolate, heap_info.total_physical_size()));
  heap_statistics->Set(
      context, toString("total_available_size"),
      v8::Integer::NewFromUnsigned(isolate, heap_info.total_available_size()));
  heap_statistics->Set(
      context, toString("used_heap_size"),
      v8::Integer::NewFromUnsigned(isolate, heap_info.used_heap_size()));
  heap_statistics->Set(
      context, toString("heap_size_limit"),
      v8::Integer::NewFromUnsigned(isolate, heap_info.heap_size_limit()));
  heap_statistics->Set(
      context, toString("malloced_memory"),
      v8::Integer::NewFromUnsigned(isolate, heap_info.malloced_memory()));
  heap_statistics->Set(
      context, toString("peak_malloced_memory"),
      v8::Integer::NewFromUnsigned(isolate, heap_info.peak_malloced_memory()));
  heap_statistics->Set(
      context, toString("does_zap_garbage"),
      v8::Integer::NewFromUnsigned(isolate, heap_info.does_zap_garbage()));
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
  };
  OperationConfig operas[]{
      {"createContext", 0, CreateContextOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
      {"createContextAsync", 0, CreateContextAsyncOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
      {"createInspectorSession", 0, CreateInspectorSessionOperationCallback,
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
