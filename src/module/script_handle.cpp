#include "script_handle.h"

#include "../isolate/external_data.h"
#include "../isolate/isolate_holder.h"
#include "../isolate/task.h"
#include "../utils/utils.h"
#include "isolate_handle.h"

namespace svm {

class CompileScriptTask final : public SyncTask<ScriptId> {
 public:
  CompileScriptTask(IsolateHandle* isolate_handle,
                    std::string& script,
                    std::string& filename)
      : isolate_handle_(isolate_handle),
        script_(std::move(script)),
        filename_(std::move(filename)) {}
  ~CompileScriptTask() override = default;

  void Run() override {
    v8::Isolate* isolate = isolate_handle_->GetIsolateSel();
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Context> context =
        isolate_handle_->GetContextHandle()->GetContext();
    v8::Context::Scope scope(context);

    v8::ScriptOrigin origin(isolate, toString(isolate, filename_));
    v8::ScriptCompiler::Source source(toString(isolate, script_), origin);

    v8::Local<v8::UnboundScript> unbound_script;
    if (v8::ScriptCompiler::CompileUnboundScript(isolate, &source)
            .ToLocal(&unbound_script)) {
      isolate_handle_->GetIsolateHolder()->CreateUnboundScript(unbound_script);
      SetResult(*unbound_script);
    } else {
      SetResult(nullptr);
    }
  }

 private:
  cppgc::WeakMember<IsolateHandle> isolate_handle_;
  std::string script_;
  std::string filename_;
};

class CompileScriptAsyncTask final : public AsyncTask {
 public:
  CompileScriptAsyncTask(std::unique_ptr<AsyncInfo> info,
                         IsolateHandle* isolate_handle,
                         std::string script,
                         std::string filename)
      : AsyncTask(std::move(info)),
        isolate_handle_(isolate_handle),
        script_(std::move(script)),
        filename_(std::move(filename)) {}
  ~CompileScriptAsyncTask() override = default;

  void Run() override {
    v8::Isolate* isolate = isolate_handle_->GetIsolateSel();
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Context> context =
        isolate_handle_->GetContextHandle()->GetContext();
    v8::Context::Scope scope(context);

    v8::ScriptOrigin origin(isolate, toString(isolate, filename_));
    v8::ScriptCompiler::Source source(toString(isolate, script_), origin);

    v8::Local<v8::UnboundScript> unbound_script;
    if (v8::ScriptCompiler::CompileUnboundScript(isolate, &source)
            .ToLocal(&unbound_script)) {
    } else {
    }
  }

 private:
  cppgc::Member<IsolateHandle> isolate_handle_;
  std::string script_;
  std::string filename_;
};

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
  cppgc::WeakMember<ContextHandle> context_handle_;
  cppgc::WeakMember<ScriptHandle> script_handle_;
};

ScriptHandle* ScriptHandle::Create(IsolateHandle* isolate_handle,
                                   std::string script,
                                   std::string filename) {
  auto task =
      std::make_unique<CompileScriptTask>(isolate_handle, script, filename);
  std::future<ScriptId> future = task->GetFuture();
  isolate_handle->PostTaskToSel(std::move(task));
  ScriptId address = future.get();

  if (!address) {
    return nullptr;
  }

  v8::Isolate* isolate = isolate_handle->GetIsolatePar();
  ScriptHandle* script_handle = MakeCppGcObject<GC::kSpecified, ScriptHandle>(
      isolate_handle->GetIsolatePar(), isolate_handle, address);
  NewInstance<V8ScriptHandle>(isolate, isolate->GetCurrentContext(),
                              script_handle);
  return script_handle;
}

ScriptHandle::ScriptHandle(IsolateHandle* isolate_handle, ScriptId address)
    : isolate_handle_(isolate_handle), address_(address) {}
ScriptHandle::~ScriptHandle() = default;

v8::Local<v8::UnboundScript> ScriptHandle::GetUnboundScript() const {
  return isolate_handle_->GetScript(address_);
}

std::pair<uint8_t*, size_t> ScriptHandle::Run(ContextHandle* context_handle) {
  auto task = std::make_unique<ScriptRunTask>(context_handle, this);
  auto future = task->GetFuture();
  isolate_handle_->PostTaskToSel(std::move(task));
  return future.get();
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
