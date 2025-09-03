#include "module/session_handle.h"

#include <cppgc/visitor.h>

#include "base/logger.h"
#include "isolate/isolate_holder.h"
#include "module/context_handle.h"
#include "module/isolate_handle.h"

namespace svm {

SessionHandle::SessionHandle(IsolateHandle* isolate_handle)
    : isolate_handle_{isolate_handle},
      isolate_holder_{isolate_handle->GetIsolateHolder()} {
  LOG_DEBUG("Session handle create.");
}
SessionHandle::~SessionHandle() {
  LOG_DEBUG("Session handle delete.");
}

IsolateHandle* SessionHandle::GetIsolateHandle() const {
  return isolate_handle_.Get();
}
std::shared_ptr<IsolateHolder> SessionHandle::GetIsolateHolder() const {
  return isolate_holder_.lock();
}

void SessionHandle::Connect(int port) const {
  class ConnectAgentTask : public v8::Task {
   public:
    ConnectAgentTask(const std::shared_ptr<IsolateHolder>& isolate_holder,
                     int port)
        : isolate_holder_{isolate_holder}, port_{port} {}
    ~ConnectAgentTask() override = default;

    void Run() override {
      static_cast<UVScheduler<Scheduler::Type::kSelf>*>(
          isolate_holder_->GetSchedulerSel())
          ->AgentConnect(port_);
    }

   private:
    std::shared_ptr<IsolateHolder> isolate_holder_;
    int port_;
  };

  isolate_holder_.lock()->PostTaskToSel(
      std::make_unique<ConnectAgentTask>(isolate_holder_.lock(), port),
      Scheduler::TaskType::kInterrupt);
}
void SessionHandle::AddContext(const ContextHandle* context_handle,
                               std::string name) const {
  class AddContextTask : public SyncTask<bool> {
   public:
    AddContextTask(const std::shared_ptr<IsolateHolder>& isolate_holder,
                   v8::Context* const address,
                   std::string name)
        : isolate_holder_{isolate_holder},
          address_{address},
          name_{std::move(name)} {}
    ~AddContextTask() override = default;

    void Run() override {
      v8::Isolate* isolate{v8::Isolate::GetCurrent()};
      v8::HandleScope handle_scope{isolate};

      v8::Local context{isolate_holder_->GetContext(address_)};
      static_cast<UVScheduler<Scheduler::Type::kSelf>*>(
          isolate_holder_->GetSchedulerSel())
          ->AgentAddContext(context, std::move(name_));
      SetResult(true);
    }

   private:
    std::shared_ptr<IsolateHolder> isolate_holder_;
    v8::Context* const address_{};
    std::string name_;
  };

  auto waiter{base::LazyWaiter<bool>{}};
  auto task{std::make_unique<AddContextTask>(
      isolate_holder_.lock(), context_handle->GetContextId(), std::move(name))};
  task->SetWaiter(&waiter);
  isolate_holder_.lock()->PostTaskToSel(std::move(task),
                                        Scheduler::TaskType::kInterrupt);
  waiter.Wait();
}
void SessionHandle::Dispose() const {
  class DisconnectAgentTask : public v8::Task {
   public:
    explicit DisconnectAgentTask(
        const std::shared_ptr<IsolateHolder>& isolate_holder)
        : isolate_holder_{isolate_holder} {}
    ~DisconnectAgentTask() override = default;

    void Run() override {
      static_cast<UVScheduler<Scheduler::Type::kSelf>*>(
          isolate_holder_->GetSchedulerSel())
          ->AgentDisconnect();
    }

   private:
    std::shared_ptr<IsolateHolder> isolate_holder_;
  };

  isolate_holder_.lock()->PostTaskToSel(
      std::make_unique<DisconnectAgentTask>(isolate_holder_.lock()),
      Scheduler::TaskType::kInterrupt);
}

void SessionHandle::Trace(cppgc::Visitor* visitor) const {
  visitor->Trace(isolate_handle_);
  ScriptWrappable::Trace(visitor);
}

///////////////////////////////////////////////////////////////////////////////
void ConnectOperationCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::Local context{isolate->GetCurrentContext()};

  v8::Local receiver{info.This()};
  SessionHandle* session_handle{
      ScriptWrappable::Unwrap<SessionHandle>(receiver)};
  session_handle->Connect(info[0]->Int32Value(context).FromJust());
}
void AddContextOperationCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local receiver{info.This()};
  SessionHandle* session_handle{
      ScriptWrappable::Unwrap<SessionHandle>(receiver)};
  ContextHandle* context_handle{
      ScriptWrappable::Unwrap<ContextHandle>(info[0].As<v8::Object>())};
  session_handle->AddContext(
      context_handle, *v8::String::Utf8Value(info.GetIsolate(), info[1]));
}
void DisposeOperationCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local receiver{info.This()};
  SessionHandle* session_handle{
      ScriptWrappable::Unwrap<SessionHandle>(receiver)};
  session_handle->Dispose();
}

void V8SessionHandle::InstallInterfaceTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::Template> interface_template) {
  ConstructConfig constructor{"Session", 0, nullptr};
  InstallConstructor(isolate, interface_template, constructor);

  OperationConfig operas[]{
      {"connect", 1, ConnectOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
      {"addContext", 2, AddContextOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
      {"dispose", 0, DisposeOperationCallback,
       v8::PropertyAttribute::DontDelete, Dependence::kPrototype},
  };

  v8::Local signature{v8::Local<v8::Signature>::Cast(interface_template)};
  InstallOperations(isolate, interface_template, signature, operas);
}

const WrapperTypeInfo V8SessionHandle::wrapper_type_info_{
    "Session", V8SessionHandle::InstallInterfaceTemplate, nullptr};

}  // namespace svm
