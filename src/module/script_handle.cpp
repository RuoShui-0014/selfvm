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
    if (script->Run(context).ToLocal(&result)) {
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
  ContextId context_id_;
  ScriptId script_id_;
};

ScriptHandle::ScriptHandle(IsolateHandle* isolate_handle, ScriptId address)
    : isolate_handle_{isolate_handle},
      isolate_holder_{isolate_handle->GetIsolateHolder()},
      address_{address} {}
ScriptHandle::~ScriptHandle() {
#ifdef DEBUG
  std::cout << "~ScriptHandle()" << std::endl;
#endif
}

v8::Local<v8::UnboundScript> ScriptHandle::GetScript() const {
  return isolate_holder_->GetScript(address_);
}

std::pair<uint8_t*, size_t> ScriptHandle::Run(ContextHandle* context_handle) {
  auto task{std::make_unique<ScriptRunTask>(
      isolate_holder_, context_handle->GetContextId(), address_)};
  auto future{task->GetFuture()};
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

  v8::Isolate* isolate{info.GetIsolate()};
  if (!IsInstance<V8ContextHandle>(isolate, info[0])) {
    isolate->ThrowException(v8::Exception::TypeError(
        toString(isolate, "The first argument must be Context.")));
    return;
  }

  v8::HandleScope scope{isolate};
  v8::Local context{isolate->GetCurrentContext()};
  v8::Local receiver{info.This()};

  ScriptHandle* script_handle{ScriptWrappable::Unwrap<ScriptHandle>(receiver)};
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

  v8::Local signature{v8::Local<v8::Signature>::Cast(interface_template)};
  // InstallAttributes(isolate, interface_template, signature, attrs);
  InstallOperations(isolate, interface_template, signature, operas);
}

const WrapperTypeInfo V8ScriptHandle::wrapper_type_info_{
    "Script", V8ScriptHandle::InstallInterfaceTemplate, nullptr};

}  // namespace svm
