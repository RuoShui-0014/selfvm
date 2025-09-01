#include "isolate/inspector_agent.h"

#include "base/logger.h"
#include "isolate/scheduler.h"

#if defined(DEBUG_FLAG)
#include <iostream>
#endif

namespace svm {

namespace {
std::mutex mutex_port_scheduler_map;
std::unordered_map<int, Scheduler*> g_port_scheduler_map;
}  // namespace
void InspectorAgent::RegisterAgent(int port, Scheduler* scheduler) {
  std::lock_guard lock(mutex_port_scheduler_map);
  g_port_scheduler_map.emplace(std::make_pair(port, scheduler));
}
void InspectorAgent::UnregisterAgent(int port) {
  std::lock_guard lock(mutex_port_scheduler_map);
  g_port_scheduler_map.erase(port);
}
Scheduler* InspectorAgent::GetAgent(int port) {
  std::lock_guard lock(mutex_port_scheduler_map);
  if (const auto it = g_port_scheduler_map.find(port);
      it != g_port_scheduler_map.end()) {
    return it->second;
  }
  return nullptr;
}

InspectorAgent::InspectorAgent(v8::Isolate* isolate, Scheduler* scheduler)
    : isolate_{isolate}, scheduler_{scheduler} {
  LOG_INFO("Inspector agent create.");
}
InspectorAgent::~InspectorAgent() {
  Disconnect();

  LOG_INFO("Inspector agent delete.");
}

void InspectorAgent::Connect(int port) {
  if (!UVSchedulerPar::nodejs_scheduler) {
    return;
  }

  inspector_ = v8_inspector::V8Inspector::create(isolate_, this);
  session_ = inspector_->connect(
      1, this, v8_inspector::StringView(),
      v8_inspector::V8Inspector::ClientTrustLevel::kFullyTrusted,
      v8_inspector::V8Inspector::SessionPauseState::kWaitingForDebugger);

  class ConnectTask : public v8::Task {
   public:
    explicit ConnectTask(int port) : port_{port} {}
    ~ConnectTask() override = default;

    void Run() override {
      v8::Isolate* isolate{v8::Isolate::GetCurrent()};
      v8::HandleScope scope{isolate};
      v8::Local context{isolate->GetCurrentContext()};

      v8::Local<v8::Value> callback{};
      if (context->Global()
              ->Get(context, toString(isolate, "sessionConnect"))
              .ToLocal(&callback) &&
          callback->IsFunction()) {
        v8::Local<v8::Value> args[]{v8::Integer::New(isolate, port_)};
        callback.As<v8::Function>()->Call(context, context->Global(), 1, args);
      }
    }

   private:
    int port_;
  };
  UVSchedulerPar::nodejs_scheduler->PostTask(
      std::make_unique<ConnectTask>(port), Scheduler::TaskType::kInterrupt);
  port_ = port;
  RegisterAgent(port, scheduler_);
  is_connected_ = true;
}
void InspectorAgent::Disconnect() {
  if (!UVSchedulerPar::nodejs_scheduler || !is_connected_) {
    return;
  }

  class DisconnectTask : public v8::Task {
   public:
    explicit DisconnectTask(int port) : port_{port} {}
    ~DisconnectTask() override = default;

    void Run() override {
      v8::Isolate* isolate{v8::Isolate::GetCurrent()};
      v8::HandleScope scope{isolate};
      v8::Local context{isolate->GetCurrentContext()};

      v8::Local<v8::Value> callback{};
      if (context->Global()
              ->Get(context, toString(isolate, "sessionDisconnect"))
              .ToLocal(&callback) &&
          callback->IsFunction()) {
        v8::Local<v8::Value> args[]{v8::Integer::New(isolate, port_)};
        callback.As<v8::Function>()->Call(context, context->Global(), 1, args);
      }
    }

   private:
    int port_;
  };
  UVSchedulerPar::nodejs_scheduler->PostTask(
      std::make_unique<DisconnectTask>(port_), Scheduler::TaskType::kInterrupt);
  UnregisterAgent(port_);
  is_connected_ = false;
}

void InspectorAgent::AddContext(v8::Local<v8::Context> context,
                                const std::string& name) const {
  if (!is_connected_) {
    return;
  }
  v8_inspector::StringView contextName{
      reinterpret_cast<const uint8_t*>(name.data()), name.length()};
  inspector_->contextCreated(
      v8_inspector::V8ContextInfo(context, 1, contextName));
}

void InspectorAgent::DispatchProtocolMessage(const std::string& message) const {
  session_->dispatchProtocolMessage(v8_inspector::StringView{
      reinterpret_cast<const uint8_t*>(message.c_str()), message.length()});
}
void InspectorAgent::Dispose() {
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

  while (waiting_for_frontend_.load() && waiting_for_resume_.load()) {
    auto* scheduler{static_cast<UVSchedulerSel*>(scheduler_)};
    scheduler->FlushInterruptTasks();
  }

  running_nested_loop_.store(false);
}
void InspectorAgent::quitMessageLoopOnPause() {
  waiting_for_resume_.store(false);
}
void InspectorAgent::runIfWaitingForDebugger(int context_group_id) {
  waiting_for_frontend_.store(true);
}

void InspectorAgent::sendResponse(
    int callId,
    std::unique_ptr<v8_inspector::StringBuffer> message) {
  class SendResponseTask : public v8::Task {
   public:
    explicit SendResponseTask(
        int port,
        std::unique_ptr<v8_inspector::StringBuffer> message)
        : port_{port}, message_{std::move(message)} {}
    ~SendResponseTask() override = default;

    void Run() override {
      v8::Isolate* isolate{v8::Isolate::GetCurrent()};
      v8::HandleScope handle_scope{isolate};
      v8::Local context{isolate->GetCurrentContext()};
      v8::Context::Scope context_scope{context};

      auto stringView{message_->string()};
      int length{static_cast<int>(stringView.length())};
      v8::Local message{
          (stringView.is8Bit()
               ? v8::String::NewFromOneByte(isolate, stringView.characters8(),
                                            v8::NewStringType::kNormal, length)
               : v8::String::NewFromTwoByte(isolate, stringView.characters16(),
                                            v8::NewStringType::kNormal, length))
              .ToLocalChecked()};
      v8::Local<v8::Value> callback{};
      if (context->Global()
              ->Get(context, toString(isolate, "sessionOnResponse"))
              .ToLocal(&callback)) {
        v8::Local<v8::Value> args[]{v8::Integer::New(isolate, port_), message};
        callback.As<v8::Function>()->Call(context, context->Global(), 2, args);
      }
    }

   private:
    int port_;
    std::unique_ptr<v8_inspector::StringBuffer> message_;
  };
  if (UVSchedulerPar::nodejs_scheduler) {
    UVSchedulerPar::nodejs_scheduler->PostTask(
        std::make_unique<SendResponseTask>(port_, std::move(message)),
        Scheduler::TaskType::kInterrupt);
  }
}
void InspectorAgent::sendNotification(
    std::unique_ptr<v8_inspector::StringBuffer> message) {
  class SendNotificationTask : public v8::Task {
   public:
    explicit SendNotificationTask(
        int port,
        std::unique_ptr<v8_inspector::StringBuffer> message)
        : port_{port}, message_{std::move(message)} {}
    ~SendNotificationTask() override = default;

    void Run() override {
      v8::Isolate* isolate{v8::Isolate::GetCurrent()};
      v8::HandleScope handle_scope{isolate};
      v8::Local context{isolate->GetCurrentContext()};
      v8::Context::Scope context_scope{context};

      auto stringView{message_->string()};
      int length{static_cast<int>(stringView.length())};
      v8::Local message{
          (stringView.is8Bit()
               ? v8::String::NewFromOneByte(isolate, stringView.characters8(),
                                            v8::NewStringType::kNormal, length)
               : v8::String::NewFromTwoByte(isolate, stringView.characters16(),
                                            v8::NewStringType::kNormal, length))
              .ToLocalChecked()};
      v8::Local<v8::Value> callback{};
      if (context->Global()
              ->Get(context, toString(isolate, "sessionOnNotification"))
              .ToLocal(&callback)) {
        v8::Local<v8::Value> args[]{v8::Integer::New(isolate, port_), message};
        callback.As<v8::Function>()->Call(context, context->Global(), 2, args);
      }
    }

   private:
    int port_;
    std::unique_ptr<v8_inspector::StringBuffer> message_;
  };
  if (UVSchedulerPar::nodejs_scheduler) {
    UVSchedulerPar::nodejs_scheduler->PostTask(
        std::make_unique<SendNotificationTask>(port_, std::move(message)),
        Scheduler::TaskType::kInterrupt);
  }
}
void InspectorAgent::flushProtocolNotifications() {}

}  // namespace svm
