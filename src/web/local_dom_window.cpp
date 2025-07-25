#include "local_dom_window.h"

#include "../isolate/isolate_holder.h"
#include "../module/isolate_handle.h"
#include "../utils/utils.h"

namespace svm {

LocalDOMWindow::LocalDOMWindow(IsolateHolder* isolate_holder)
    : isolate_holder_{isolate_holder} {}
LocalDOMWindow::~LocalDOMWindow() = default;

void LocalDOMWindow::PostTaskToSel(std::unique_ptr<v8::Task> task) const {
  isolate_holder_->PostTaskToSel(std::move(task));
}
void LocalDOMWindow::PostTaskToPar(std::unique_ptr<v8::Task> task) const {
  isolate_holder_->PostTaskToPar(std::move(task));
}
void LocalDOMWindow::PostDelayTaskToSel(std::unique_ptr<v8::Task> task,
                                        double delay) const {
  isolate_holder_->PostDelayedTaskToSel(std::move(task), delay);
}
void LocalDOMWindow::PostDelayTaskToPar(std::unique_ptr<v8::Task> task,
                                        double delay) const {
  isolate_holder_->PostDelayedTaskToPar(std::move(task), delay);
}

//////////////////////////////////////////////////////////////////////////////////////
void WindowConstructCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  info.GetIsolate()->ThrowException(v8::Exception::TypeError(
      toString(info.GetIsolate(), "Illegal constructor")));
}

void WindowAttributeGetCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> receiver = info.This();
  LocalDOMWindow* blink_receiver =
      ScriptWrappable::Unwrap<LocalDOMWindow>(receiver);
  LocalDOMWindow* return_value = blink_receiver->window();
  V8SetReturnValue(info, return_value);
}

void AtobOperationCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> receiver = info.This();

  LocalDOMWindow* blink_receiver =
      ScriptWrappable::Unwrap<LocalDOMWindow>(receiver);
  LocalDOMWindow* return_value = blink_receiver->window();

  // info.GetReturnValue().Set();
}

void BtoaOperationCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  if (info.Length() < 1) {
    isolate->ThrowException(v8::Exception::TypeError(
        toString(info.GetIsolate(),
                 "Failed to execute 'btoa' on 'Window': 1 argument required, "
                 "but only 0 present.")));
    return;
  }

  char* str = *v8::String::Utf8Value(isolate, info[0]);
  info.GetReturnValue().Set(
      node::Encode(info.GetIsolate(), str, strlen(str), node::BASE64));
}

void SetTimeoutOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  class SetTimeoutTask : public v8::Task {
   public:
    SetTimeoutTask(v8::Isolate* isolate,
                   v8::Local<v8::Context> context,
                   v8::Local<v8::Function> callback)
        : isolate_{isolate}, context_{context}, callback_(isolate, callback) {}
    ~SetTimeoutTask() override = default;

    void Run() override {
      v8::HandleScope handle_scope(isolate_);
      v8::Local<v8::Context> context = context_.Get(isolate_);
      v8::Context::Scope context_scope(context);

      callback_.Get(isolate_)->Call(context, context->Global(), 0, nullptr);
    }

   private:
    v8::Isolate* isolate_;
    RemoteHandle<v8::Context> context_;
    RemoteHandle<v8::Function> callback_;
  };
  v8::Isolate* isolate = info.GetIsolate();
  if (info.Length() < 2) {
    isolate->ThrowException(v8::Exception::TypeError(
        toString(info.GetIsolate(),
                 "Failed to execute 'btoa' on 'Window': 1 argument required, "
                 "but only 0 present.")));
    return;
  }
  if (!info[0]->IsFunction() && !info[1]->IsNumber()) {
    return;
  }

  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Object> receiver = info.This();
  LocalDOMWindow* local_dom_window =
      ScriptWrappable::Unwrap<LocalDOMWindow>(receiver);
  auto task = std::make_unique<SetTimeoutTask>(isolate, context,
                                               info[0].As<v8::Function>());
  local_dom_window->PostDelayTaskToSel(
      std::move(task), info[1].As<v8::Number>()->Value() / 1000);
}

void SetIntervalOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  class SetIntervalTask : public v8::Task {
   public:
    SetIntervalTask(v8::Isolate* isolate,
                    v8::Local<v8::Context> context,
                    v8::Local<v8::Function> callback,
                    double delay)
        : isolate_{isolate},
          delay_{delay},
          context_{context},
          callback_(isolate, callback) {}
    ~SetIntervalTask() override = default;

    void Run() override {
      v8::HandleScope handle_scope(isolate_);
      v8::Local<v8::Context> context = context_.Get(isolate_);
      v8::Context::Scope context_scope(context);

      callback_.Get(isolate_)->Call(context, context->Global(), 0, nullptr);

      v8::Local<v8::Object> receiver = context->Global();
      LocalDOMWindow* local_dom_window =
          ScriptWrappable::Unwrap<LocalDOMWindow>(receiver);
      auto task = std::make_unique<SetIntervalTask>(
          isolate_, context, callback_.Get(isolate_), delay_);
      local_dom_window->PostDelayTaskToSel(std::move(task), delay_);
    }

   private:
    v8::Isolate* isolate_;
    double delay_;
    RemoteHandle<v8::Context> context_;
    RemoteHandle<v8::Function> callback_;
  };
  v8::Isolate* isolate = info.GetIsolate();
  if (info.Length() < 2) {
    isolate->ThrowException(v8::Exception::TypeError(
        toString(info.GetIsolate(),
                 "Failed to execute 'btoa' on 'Window': 1 argument required, "
                 "but only 0 present.")));
    return;
  }
  if (!info[0]->IsFunction() && !info[1]->IsNumber()) {
    return;
  }
  double delay = info[1].As<v8::Number>()->Value() / 1000;

  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Object> receiver = info.This();
  LocalDOMWindow* local_dom_window =
      ScriptWrappable::Unwrap<LocalDOMWindow>(receiver);
  auto task = std::make_unique<SetIntervalTask>(
      isolate, context, info[0].As<v8::Function>(), delay);
  local_dom_window->PostDelayTaskToSel(std::move(task), delay);
}

void IsolateExposedConstructCallback(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  info.GetReturnValue().Set(GetExposedInterfaceObject(
      info.GetIsolate(), info.Holder(), V8IsolateHandle::GetWrapperTypeInfo()));
}

void WindowExposedConstructCallback(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  info.GetReturnValue().Set(GetExposedInterfaceObject(
      info.GetIsolate(), info.Holder(), V8Window::GetWrapperTypeInfo()));
}

void V8Window::InstallInterfaceTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::Template> interface_template) {
  ConstructConfig constructor{"Window", 0, WindowConstructCallback};
  AttributeConfig attrs[]{
      {"window", WindowAttributeGetCallback, nullptr,
       v8::PropertyAttribute::ReadOnly, Dependence::kInstance},
  };
  OperationConfig operas[]{
      {"atob", 1, AtobOperationCallback, v8::PropertyAttribute::DontDelete,
       Dependence::kInstance},
      {"btoa", 1, BtoaOperationCallback, v8::PropertyAttribute::DontDelete,
       Dependence::kInstance},
      {"setTimeout", 2, SetTimeoutOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kInstance},
      {"setInterval", 2, SetIntervalOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kInstance},
  };
  ExposedConstructConfig exposedConstructs[]{
      {"Isolate", IsolateExposedConstructCallback, Dependence::kInstance},
      {"Window", WindowExposedConstructCallback, Dependence::kInstance},
  };

  InstallConstructor(isolate, interface_template, constructor);

  v8::Local<v8::Signature> signature =
      v8::Local<v8::Signature>::Cast(interface_template);
  InstallAttributes(isolate, interface_template, signature, attrs);
  InstallOperations(isolate, interface_template, signature, operas);
  InstallExposedConstructs(isolate, interface_template, exposedConstructs);
}

const WrapperTypeInfo V8Window::wrapper_type_info_{
    "Window", V8Window::InstallInterfaceTemplate, nullptr};

}  // namespace svm
