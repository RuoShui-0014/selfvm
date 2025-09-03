#include "base/logger.h"
#include "isolate/inspector_agent.h"
#include "isolate/per_isolate_data.h"
#include "isolate/platform_delegate.h"
#include "isolate/scheduler.h"
#include "module/isolate_handle.h"
#include "net/tcp_stream.h"
#include "utils/utils.h"

namespace svm {

void NodeGcOperationCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  info.GetIsolate()->LowMemoryNotification();
}

void SessionDispatchMessageCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  class SessionDispatchMessage : public v8::Task {
   public:
    SessionDispatchMessage(Scheduler* scheduler, std::string message)
        : scheduler_{scheduler}, message_{std::move(message)} {}
    ~SessionDispatchMessage() override = default;

    void Run() override {
      static_cast<UVSchedulerSel*>(scheduler_)
          ->AgentDispatchProtocolMessage(message_);
    }

   private:
    Scheduler* scheduler_;
    std::string message_;
  };

  if (info.Length() < 2 && !info[0]->IsNumber() && !info[1]->IsString()) {
    return;
  }
  v8::Isolate* isolate{info.GetIsolate()};
  v8::Local context{isolate->GetCurrentContext()};

  int port{info[0]->Int32Value(context).FromJust()};
  std::string message{*v8::String::Utf8Value(isolate, info[1])};
  if (Scheduler * scheduler{InspectorAgent::GetAgent(port)}) {
    scheduler->PostTask(
        std::make_unique<SessionDispatchMessage>(scheduler, std::move(message)),
        Scheduler::TaskType::kInterrupt);
  }
}

void SessionDisposeCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  class DisposeAgentTask : public v8::Task {
   public:
    explicit DisposeAgentTask(Scheduler* scheduler) : scheduler_{scheduler} {}
    ~DisposeAgentTask() override = default;

    void Run() override {
      static_cast<UVSchedulerSel*>(scheduler_)->AgentDispose();
    }

   private:
    Scheduler* scheduler_;
  };

  if (info.Length() < 1 && !info[0]->IsNumber()) {
    return;
  }
  v8::Isolate* isolate{info.GetIsolate()};
  v8::Local context{isolate->GetCurrentContext()};

  const int port{info[0]->Int32Value(context).FromJust()};
  if (Scheduler* scheduler = InspectorAgent::GetAgent(port)) {
    scheduler->PostTask(std::make_unique<DisposeAgentTask>(scheduler),
                        Scheduler::TaskType::kInterrupt);
  }
}

void CreateNodeEx(v8::Isolate* isolate,
                  v8::Local<v8::Context> context,
                  v8::Local<v8::Object> exports) {
  ObjectAddFunction(exports, "gc", 0, NodeGcOperationCallback);
  ObjectAddFunction(exports, "sessionDispatchMessage", 2,
                    SessionDispatchMessageCallback);
  ObjectAddFunction(exports, "sessionDispose", 1, SessionDisposeCallback);

  // IsolateHandle construct
  exports->Set(context, toString(isolate, "Isolate"),
               NewFunction<V8IsolateHandle>(isolate, context));
}

struct NodeData {
  UVSchedulerPar* g_scheduler_par{nullptr};
  PerIsolateData* g_per_isolate_data{nullptr};
#ifdef DEBUG_FLAG
  TcpStream* g_tcp_stream{nullptr};
#endif
} node_data;

void Initialize(v8::Local<v8::Object> exports) {
#ifdef DEBUG_FLAG
  base::Logger::Initialize("./log.txt", base::Logger::Level::kDebug);
#else
  base::Logger::Initialize("./log.txt", base::Logger::Level::kInfo);
#endif
  v8::Isolate* isolate{v8::Isolate::GetCurrent()};
  v8::Local context{isolate->GetCurrentContext()};

  // init nodejs env
  PlatformDelegate::InitializeDelegate();
  node_data.g_scheduler_par =
      new UVSchedulerPar(isolate, node::GetCurrentEventLoop(isolate));
  node_data.g_per_isolate_data =
      new PerIsolateData(isolate, node_data.g_scheduler_par);

  //
  CreateNodeEx(isolate, context, exports);

  /* test */
// #ifdef DEBUG_FLAG
//   node_data.g_tcp_stream = new TcpStream{node::GetCurrentEventLoop(isolate)};
//   node_data.g_tcp_stream->DomainConnect("www.runoob.com", 80);
// #endif

  // release some object
  node::AddEnvironmentCleanupHook(
      isolate,
      [](void* arg) {
#ifdef DEBUG_FLAG
        const auto [g_scheduler_par, g_per_isolate_data,
                    g_tcp_stream]{*static_cast<NodeData*>(arg)};
#else
        const auto [g_scheduler_par,
                    g_per_isolate_data]{*static_cast<NodeData*>(arg)};
#endif

        delete g_scheduler_par;
        delete g_per_isolate_data;

        LOG_INFO("Nodejs clear success.");

#ifdef DEBUG_FLAG
        delete g_tcp_stream;
#endif
      },
      &node_data);

  LOG_INFO("Self-vm load.");
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize);

}  // namespace svm
