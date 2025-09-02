#include "local_dom_window.h"

#include "isolate/isolate_holder.h"
#include "module/isolate_handle.h"
#include "tool/tools.h"
#include "utils/utils.h"

namespace svm {

class TimerTask : public v8::Task {
 public:
  TimerTask(IsolateHolder* isolate_holder,
            v8::Isolate* isolate,
            v8::Local<v8::Context> context,
            v8::Local<v8::Function> callback)
      : isolate_holder_{isolate_holder},
        isolate_{isolate},
        context_{context},
        callback_{isolate, callback} {
    isolate_holder_->GetSchedulerPar()->Ref();
  }
  ~TimerTask() override { isolate_holder_->GetSchedulerPar()->Unref(); }

  void Run() override {
    v8::HandleScope handle_scope{isolate_};
    v8::Local context{context_.Get(isolate_)};
    v8::Context::Scope context_scope{context};

    callback_.Get(isolate_)->Call(context, context->Global(), 0, nullptr);
  }

 private:
  IsolateHolder* isolate_holder_;
  v8::Isolate* isolate_;
  RemoteHandle<v8::Context> context_;
  RemoteHandle<v8::Function> callback_;
};

LocalDOMWindow::LocalDOMWindow(IsolateHolder* isolate_holder)
    : isolate_holder_{isolate_holder} {}
LocalDOMWindow::~LocalDOMWindow() = default;

void LocalDOMWindow::PostTaskToSel(std::unique_ptr<v8::Task> task,
                                   Scheduler::TaskType type) const {
  isolate_holder_->PostTaskToSel(std::move(task), type);
}
uint32_t LocalDOMWindow::PostTimeoutTaskToSel(std::unique_ptr<v8::Task> task,
                                              uint64_t ms) const {
  return isolate_holder_->PostDelayedTaskToSel(std::move(task), ms,
                                               Timer::Type::ktimeout);
}
uint32_t LocalDOMWindow::PostIntervalTaskToSel(std::unique_ptr<v8::Task> task,
                                               uint64_t ms) const {
  return isolate_holder_->PostDelayedTaskToSel(std::move(task), ms,
                                               Timer::Type::ktimeout);
}
void LocalDOMWindow::PostTaskToPar(std::unique_ptr<v8::Task> task,
                                   Scheduler::TaskType type) const {
  isolate_holder_->PostTaskToPar(std::move(task), type);
}
void LocalDOMWindow::PostDelayTaskToPar(std::unique_ptr<v8::Task> task,
                                        double delay) const {
  isolate_holder_->PostDelayedTaskToPar(std::move(task), delay);
}
void LocalDOMWindow::ClearTimeout(uint32_t id) const {
  isolate_holder_->GetTimerManagerSel()->StopTimer(id);
}
void LocalDOMWindow::ClearInterval(uint32_t id) const {
  isolate_holder_->GetTimerManagerSel()->StopTimer(id);
}

//////////////////////////////////////////////////////////////////////////////////////
void WindowConstructCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  isolate->ThrowException(
      v8::Exception::TypeError(toString(isolate, "Illegal constructor")));
}

void WindowAttributeGetCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local receiver{info.This()};
  LocalDOMWindow* blink_receiver{
      ScriptWrappable::Unwrap<LocalDOMWindow>(receiver)};
  LocalDOMWindow* return_value{blink_receiver->window()};
  V8SetReturnValue(info, return_value);
}

void AtobOperationCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {}

void BtoaOperationCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  if (info.Length() < 1) {
    isolate->ThrowException(v8::Exception::TypeError(
        toString(info.GetIsolate(),
                 "Failed to execute 'btoa' on 'Window': 1 argument required, "
                 "but only 0 present.")));
    return;
  }

  const char* btoa_str{*v8::String::Utf8Value(isolate, info[0])};
  info.GetReturnValue().Set(node::Encode(info.GetIsolate(), btoa_str,
                                         strlen(btoa_str), node::BASE64));
}

void SetTimeoutOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
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

  v8::Local context{isolate->GetCurrentContext()};
  v8::Local receiver{info.This()};
  const LocalDOMWindow* local_dom_window{
      ScriptWrappable::Unwrap<LocalDOMWindow>(receiver)};
  auto task{std::make_unique<TimerTask>(local_dom_window->GetIsolateHolder(),
                                        isolate, context,
                                        info[0].As<v8::Function>())};
  uint32_t id{local_dom_window->PostTimeoutTaskToSel(
      std::move(task),
      info[1].As<v8::Number>()->Int32Value(context).FromMaybe(0))};
  info.GetReturnValue().Set(id);
}

void SetIntervalOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
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

  v8::Local context{isolate->GetCurrentContext()};
  v8::Local receiver{info.This()};
  LocalDOMWindow* local_dom_window{
      ScriptWrappable::Unwrap<LocalDOMWindow>(receiver)};
  auto task{std::make_unique<TimerTask>(local_dom_window->GetIsolateHolder(),
                                        isolate, context,
                                        info[0].As<v8::Function>())};
  uint32_t id{local_dom_window->PostIntervalTaskToSel(
      std::move(task),
      info[1].As<v8::Number>()->Int32Value(context).FromMaybe(0))};
  info.GetReturnValue().Set(id);
}

void ClearTimeoutOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  if (info.Length() < 1) {
    return;
  }
  if (!info[0]->IsNumber()) {
    return;
  }

  v8::Local context{isolate->GetCurrentContext()};
  v8::Local receiver{info.This()};
  LocalDOMWindow* local_dom_window{
      ScriptWrappable::Unwrap<LocalDOMWindow>(receiver)};

  int id{info[0].As<v8::Number>()->Int32Value(context).FromMaybe(0)};
  local_dom_window->ClearTimeout(id);
}

void ClearIntervalOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  if (info.Length() < 1) {
    return;
  }
  if (!info[0]->IsNumber()) {
    return;
  }

  v8::Local context{isolate->GetCurrentContext()};
  v8::Local receiver{info.This()};
  LocalDOMWindow* local_dom_window{
      ScriptWrappable::Unwrap<LocalDOMWindow>(receiver)};

  int id{info[0].As<v8::Number>()->Int32Value(context).FromMaybe(0)};
  local_dom_window->ClearTimeout(id);
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
  InstallConstructor(isolate, interface_template, constructor);

  AttributeConfig attrs[]{
      {"window", WindowAttributeGetCallback, nullptr,
       v8::PropertyAttribute::ReadOnly, Dependence::kInstance},
  };
  OperationConfig operas[]{
      // {"atob", 1, AtobOperationCallback, v8::PropertyAttribute::DontDelete,
      //  Dependence::kInstance},
      // {"btoa", 1, BtoaOperationCallback, v8::PropertyAttribute::DontDelete,
      //  Dependence::kInstance},
      {"setTimeout", 2, SetTimeoutOperationCallback,
       v8::PropertyAttribute::None, Dependence::kInstance},
      {"setInterval", 2, SetIntervalOperationCallback,
       v8::PropertyAttribute::None, Dependence::kInstance},
      {"clearTimeout", 1, ClearTimeoutOperationCallback,
       v8::PropertyAttribute::None, Dependence::kInstance},
      {"clearInterval", 1, ClearIntervalOperationCallback,
       v8::PropertyAttribute::None, Dependence::kInstance},
  };
  ExposedConstructConfig exposedConstructs[]{
      {"Isolate", IsolateExposedConstructCallback, Dependence::kInstance},
      {"Window", WindowExposedConstructCallback, Dependence::kInstance},
  };

  v8::Local signature{v8::Local<v8::Signature>::Cast(interface_template)};
  InstallAttributes(isolate, interface_template, signature, attrs);
  InstallOperations(isolate, interface_template, signature, operas);
  InstallExposedConstructs(isolate, interface_template, exposedConstructs);
}

const WrapperTypeInfo V8Window::wrapper_type_info_{
    "Window", V8Window::InstallInterfaceTemplate, nullptr};

}  // namespace svm
