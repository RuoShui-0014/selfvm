#include "session_handle.h"

#include <cppgc/visitor.h>

#ifdef DEBUG
#include <iostream>
#endif

#include "../isolate/isolate_holder.h"
#include "../isolate/platform_delegate.h"
#include "context_handle.h"
#include "isolate_handle.h"

namespace svm {

InspectorAgent::InspectorAgent(SessionHandle* session_handle,
                               v8::Isolate* isolate)
    : session_handle_(session_handle),
      isolate_(isolate),
      channel_(std::make_unique<InspectorChannel>(session_handle)) {
  connectInspector();
}
InspectorAgent::~InspectorAgent() {
  channel_.reset();
  session_.reset();
  inspector_.reset();

#ifdef DEBUG
  std::cout << "~InspectorAgent()" << std::endl;
#endif
}

void InspectorAgent::connectInspector() {
  inspector_ = v8_inspector::V8Inspector::create(isolate_, this);
  session_ = inspector_->connect(
      1, channel_.get(), v8_inspector::StringView(),
      v8_inspector::V8Inspector::ClientTrustLevel::kFullyTrusted,
      v8_inspector::V8Inspector::SessionPauseState::kWaitingForDebugger);
}

void InspectorAgent::addContext(v8::Local<v8::Context> context) const {
  v8_inspector::StringView contextName{
      reinterpret_cast<const uint8_t*>("inspector"), 9};
  inspector_->contextCreated(
      v8_inspector::V8ContextInfo(context, 1, contextName));
}

void InspectorAgent::dispatchMessage(std::string message) {
  class DispatchMessageTask : public v8::Task {
   public:
    explicit DispatchMessageTask(v8_inspector::V8InspectorSession* session,
                                 const std::string& message)
        : message_(message), session_(std::move(session)) {}
    ~DispatchMessageTask() override = default;
    void Run() override {
      session_->dispatchProtocolMessage(v8_inspector::StringView{
          reinterpret_cast<const uint8_t*>(message_.c_str()),
          message_.length()});
    }

   private:
    std::string message_;
    v8_inspector::V8InspectorSession* session_;
  };

  auto task = std::make_unique<DispatchMessageTask>(session_.get(), message);
  session_handle_->GetIsolateHolder()->PostInterruptTask(std::move(task));
}
void InspectorAgent::dispose() {
  waiting_for_frontend_.store(false);
  waiting_for_resume_.store(false);
  running_nested_loop_.store(false);
}

void InspectorAgent::runMessageLoopOnPause(int context_group_id) {
  if (running_nested_loop_.load()) {
    return;
  }
  running_nested_loop_.store(true);
  waiting_for_resume_.store(true);

  while (waiting_for_frontend_ && waiting_for_resume_) {
    UVSchedulerSel* scheduler = static_cast<UVSchedulerSel*>(
        session_handle_->isolate_handle_->GetSchedulerSel());
    scheduler->RunInterruptTasks();
  }

  running_nested_loop_.store(false);
}
void InspectorAgent::quitMessageLoopOnPause() {
  waiting_for_resume_.store(false);
}
void InspectorAgent::runIfWaitingForDebugger(int context_group_id) {
  waiting_for_frontend_.store(true);
}

InspectorChannel::InspectorChannel(SessionHandle* session_handle)
    : session_handle_(session_handle) {}
InspectorChannel::~InspectorChannel() = default;

void InspectorChannel::sendResponse(
    int callId,
    std::unique_ptr<v8_inspector::StringBuffer> message) {
  class SendResponseTask : public v8::Task {
   public:
    SendResponseTask(SessionHandle* session_handle,
                     std::unique_ptr<v8_inspector::StringBuffer> message)
        : message_(std::move(message)), session_handle_(session_handle) {}
    ~SendResponseTask() override = default;

    void Run() override {
      v8::Isolate* isolate = v8::Isolate::GetCurrent();
      v8::HandleScope handle_scope(isolate);
      v8::Local<v8::Context> context = isolate->GetCurrentContext();
      v8::Context::Scope ctx_scope(context);

      auto stringView = message_->string();
      int length = static_cast<int>(stringView.length());
      v8::Local<v8::String> message =
          (stringView.is8Bit()
               ? v8::String::NewFromOneByte(isolate, stringView.characters8(),
                                            v8::NewStringType::kNormal, length)
               : v8::String::NewFromTwoByte(isolate, stringView.characters16(),
                                            v8::NewStringType::kNormal, length))
              .ToLocalChecked();

      if (session_handle_->on_response_.has_value()) {
        v8::Local<v8::Value> args[]{message};
        session_handle_->on_response_.value().Get(isolate)->Call(
            context, context->Global(), 1, args);
      }
    }

   private:
    std::unique_ptr<v8_inspector::StringBuffer> message_;
    cppgc::Member<SessionHandle> session_handle_;
  };

  session_handle_->GetIsolateHolder()->PostTaskToPar(
      std::make_unique<SendResponseTask>(session_handle_, std::move(message)));
}
void InspectorChannel::sendNotification(
    std::unique_ptr<v8_inspector::StringBuffer> message) {
  class SendNotificationTask : public v8::Task {
   public:
    SendNotificationTask(SessionHandle* session_handle,
                         std::unique_ptr<v8_inspector::StringBuffer> message)
        : message_(std::move(message)), session_handle_(session_handle) {}
    ~SendNotificationTask() override = default;

    void Run() override {
      v8::Isolate* isolate = v8::Isolate::GetCurrent();
      v8::HandleScope handle_scope(isolate);
      v8::Local<v8::Context> context = isolate->GetCurrentContext();
      v8::Context::Scope ctx_scope(context);

      auto stringView = message_->string();
      int length = static_cast<int>(stringView.length());
      v8::Local<v8::String> message =
          (stringView.is8Bit()
               ? v8::String::NewFromOneByte(isolate, stringView.characters8(),
                                            v8::NewStringType::kNormal, length)
               : v8::String::NewFromTwoByte(isolate, stringView.characters16(),
                                            v8::NewStringType::kNormal, length))
              .ToLocalChecked();

      if (session_handle_->on_notification_.has_value()) {
        v8::Local<v8::Value> args[]{message};
        v8::Local<v8::Function> callback =
            session_handle_->on_notification_.value().Get(isolate);
        callback->Call(context, context->Global(), 1, args);
      }
    }

   private:
    std::unique_ptr<v8_inspector::StringBuffer> message_;
    cppgc::Member<SessionHandle> session_handle_;
  };
  session_handle_->GetIsolateHolder()->PostTaskToPar(
      std::make_unique<SendNotificationTask>(session_handle_,
                                             std::move(message)));
}
void InspectorChannel::flushProtocolNotifications() {}

SessionHandle::SessionHandle(IsolateHandle* isolate_handle)
    : isolate_handle_(isolate_handle),
      isolate_holder_(isolate_handle->GetIsolateHolder()),
      inspector_agent_(std::make_unique<InspectorAgent>(
          this,
          isolate_handle->GetIsolateHolder()->GetIsolateSel())) {}
SessionHandle::~SessionHandle() {
#ifdef DEBUG
  std::cout << "~SessionHandle()" << std::endl;
#endif
}

IsolateHandle* SessionHandle::GetIsolateHandle() const {
  return isolate_handle_.Get();
}
std::shared_ptr<IsolateHolder> SessionHandle::GetIsolateHolder() const {
  return isolate_holder_;
}

void SessionHandle::AddContext(v8::Local<v8::Context> context) const {
  inspector_agent_->addContext(context);
}
void SessionHandle::Release() {
  inspector_agent_.reset();
}
void SessionHandle::DispatchInspectorMessage(std::string message) {
  inspector_agent_->dispatchMessage(std::move(message));
}
void SessionHandle::AddContext(ContextHandle* context_handle) {
  class AddContextTask : public SyncTask<bool> {
   public:
    AddContextTask(SessionHandle* session_handle, v8::Context* const address)
        : session_handle_(session_handle), address_(address) {}
    ~AddContextTask() override = default;

    void Run() override {
      v8::Isolate* isolate = v8::Isolate::GetCurrent();
      v8::HandleScope handle_scope(isolate);

      v8::Local<v8::Context> context =
          session_handle_->GetIsolateHandle()->GetContext(address_);
      session_handle_->AddContext(context);
      SetResult(true);
    }

   private:
    cppgc::WeakMember<SessionHandle> session_handle_;
    v8::Context* const address_{};
  };
  auto task =
      std::make_unique<AddContextTask>(this, context_handle->GetContextId());
  std::future<bool> future = task->GetFuture();
  isolate_holder_->PostTaskToSel(std::move(task));
  future.get();
}
void SessionHandle::Dispose() {
  on_notification_.reset();
  on_response_.reset();
  inspector_agent_->dispose();
}
void SessionHandle::Trace(cppgc::Visitor* visitor) const {
  visitor->Trace(isolate_handle_);
  ScriptWrappable::Trace(visitor);
}

/////////////////////////////////////////////////////////////////////////////////
void OnResponseAttributeGetCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> receiver = info.This();
  SessionHandle* session_handle =
      ScriptWrappable::Unwrap<SessionHandle>(receiver);
  if (session_handle->on_response_.has_value()) {
    info.GetReturnValue().Set(
        session_handle->on_response_.value().Get(info.GetIsolate()));
  }
}
void OnResponseAttributeSetCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> receiver = info.This();
  SessionHandle* session_handle =
      ScriptWrappable::Unwrap<SessionHandle>(receiver);
  session_handle->on_response_.emplace(info.GetIsolate(),
                                       info[0].As<v8::Function>());
}
void OnNotificationAttributeGetCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> receiver = info.This();
  SessionHandle* session_handle =
      ScriptWrappable::Unwrap<SessionHandle>(receiver);
  if (session_handle->on_notification_.has_value()) {
    info.GetReturnValue().Set(
        session_handle->on_notification_.value().Get(info.GetIsolate()));
  }
}
void OnNotificationAttributeSetCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> receiver = info.This();
  SessionHandle* session_handle =
      ScriptWrappable::Unwrap<SessionHandle>(receiver);
  session_handle->on_notification_.emplace(info.GetIsolate(),
                                           info[0].As<v8::Function>());
}
void DispatchMessageOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> receiver = info.This();
  SessionHandle* session_handle =
      ScriptWrappable::Unwrap<SessionHandle>(receiver);
  session_handle->DispatchInspectorMessage(
      *v8::String::Utf8Value(info.GetIsolate(), info[0]));
}
void AddContextOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> receiver = info.This();
  SessionHandle* session_handle =
      ScriptWrappable::Unwrap<SessionHandle>(receiver);
  ContextHandle* context_handle =
      ScriptWrappable::Unwrap<ContextHandle>(info[0].As<v8::Object>());
  session_handle->AddContext(context_handle);
}
void DisposeOperationCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> receiver = info.This();
  SessionHandle* session_handle =
      ScriptWrappable::Unwrap<SessionHandle>(receiver);
  session_handle->Dispose();
}

void V8SessionHandle::InstallInterfaceTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::Template> interface_template) {
  ConstructConfig constructor{"Session", 0, nullptr};
  AttributeConfig attrs[]{
      {"onResponse", OnResponseAttributeGetCallback,
       OnResponseAttributeSetCallback, v8::PropertyAttribute::None,
       Dependence::kPrototype},
      {"onNotification", OnNotificationAttributeGetCallback,
       OnNotificationAttributeSetCallback, v8::PropertyAttribute::None,
       Dependence::kPrototype},
  };
  OperationConfig operas[]{
      {"dispatchMessage", 0, DispatchMessageOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
      {"addContext", 0, AddContextOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
      {"dispose", 0, DisposeOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
  };

  InstallConstructor(isolate, interface_template, constructor);

  v8::Local<v8::Signature> signature =
      v8::Local<v8::Signature>::Cast(interface_template);
  InstallAttributes(isolate, interface_template, signature, attrs);
  InstallOperations(isolate, interface_template, signature, operas);
}

const WrapperTypeInfo V8SessionHandle::wrapper_type_info_{
    "Session", V8SessionHandle::InstallInterfaceTemplate, nullptr};

}  // namespace svm
