#include "script_handle.h"

#include <iostream>

#include "../isolate/external_data.h"
#include "../isolate/isolate_holder.h"
#include "../isolate/task.h"
#include "../utils/utils.h"
#include "context_handle.h"
#include "isolate_handle.h"

namespace svm {

class ScriptRunTask final : public SyncTask<std::pair<uint8_t*, size_t>> {
 public:
  ScriptRunTask(ContextHandle* context_handle, ScriptHandle* script_handle)
      : context_handle_(context_handle), script_handle_(script_handle) {}
  ~ScriptRunTask() override = default;

  void Run() override {
    v8::Isolate* isolate = context_handle_->GetIsolateSel();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = context_handle_->GetContext();
    v8::Context::Scope scope(context);

    v8::TryCatch try_catch(isolate);
    v8::Local<v8::UnboundScript> unbound_script =
        script_handle_->GetUnboundScript();
    v8::Local<v8::Script> script = unbound_script->BindToCurrentContext();
    v8::MaybeLocal<v8::Value> maybe_result = script->Run(context);
    if (!maybe_result.IsEmpty()) {
      ExternalData::SourceData data{isolate, context,
                                    maybe_result.ToLocalChecked()};
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
  cppgc::Member<ContextHandle> context_handle_;
  cppgc::Member<ScriptHandle> script_handle_;
};

ScriptHandle::ScriptHandle(IsolateHandle* isolate_handle, ScriptId address)
    : isolate_handle_(isolate_handle),
      isolate_holder_(isolate_handle->GetIsolateHolder()),
      address_(address) {}
ScriptHandle::~ScriptHandle() {
#ifdef DEBUG
  std::cout << "~ScriptHandle()" << std::endl;
#endif
}

v8::Local<v8::UnboundScript> ScriptHandle::GetUnboundScript() const {
  return isolate_holder_->GetScript(address_);
}

std::pair<uint8_t*, size_t> ScriptHandle::Run(ContextHandle* context_handle) {
  auto task = std::make_unique<ScriptRunTask>(context_handle, this);
  auto future = task->GetFuture();
  isolate_holder_->PostTaskToSel(std::move(task));
  return future.get();
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

  v8::Isolate* isolate = info.GetIsolate();
  if (!IsInstance<V8ContextHandle>(isolate, info[0])) {
    isolate->ThrowException(v8::Exception::TypeError(
        toString(isolate, "The first argument must be Context.")));
    return;
  }

  v8::HandleScope scope(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::Local<v8::Object> receiver = info.This();
  ScriptHandle* script_handle = ScriptWrappable::Unwrap<ScriptHandle>(receiver);
  ContextHandle* context_handle =
      ScriptWrappable::Unwrap<ContextHandle>(info[0].As<v8::Object>());
  auto buff = script_handle->Run(context_handle);
  v8::Local<v8::Value> result =
      ExternalData::DeserializerSync(isolate, context, buff);
  if (!result->IsNativeError()) {
    info.GetReturnValue().Set(result);
  } else {
    isolate->ThrowException(result);
  }
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
  };

  v8::Local<v8::Signature> signature =
      v8::Local<v8::Signature>::Cast(interface_template);
  // InstallAttributes(isolate, interface_template, signature, attrs);
  InstallOperations(isolate, interface_template, signature, operas);
}

const WrapperTypeInfo V8ScriptHandle::wrapper_type_info_{
    "Script", V8ScriptHandle::InstallInterfaceTemplate, nullptr};

}  // namespace svm
