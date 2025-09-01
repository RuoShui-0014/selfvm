#pragma once

#include "isolate/scheduler.h"
#include "isolate/script_wrappable.h"
#include "utils/utils.h"
#include "vendor/v8-inspector.h"

namespace svm {

class InspectorAgent final : public v8_inspector::V8InspectorClient,
                             public v8_inspector::V8Inspector::Channel {
 public:
  static void RegisterAgent(int port, Scheduler* scheduler);
  static void UnregisterAgent(int port);
  static Scheduler* GetAgent(int port);

  explicit InspectorAgent(v8::Isolate* isolate, Scheduler* scheduler);
  ~InspectorAgent() override;

  void Connect(int port);
  void Disconnect();
  void AddContext(v8::Local<v8::Context> context,
                  const std::string& name) const;
  void DispatchProtocolMessage(const std::string& message) const;
  void Dispose();

  /* v8_inspector::V8InspectorClient override */
  void runMessageLoopOnPause(int context_group_id) override;
  void quitMessageLoopOnPause() override;
  void runIfWaitingForDebugger(int context_group_id) override;

  /* v8_inspector::V8Inspector::Channel override */
  void sendResponse(
      int callId,
      std::unique_ptr<v8_inspector::StringBuffer> message) override;
  void sendNotification(
      std::unique_ptr<v8_inspector::StringBuffer> message) override;
  void flushProtocolNotifications() override;

 private:
  v8::Isolate* isolate_{};
  Scheduler* scheduler_{};
  int port_{0};
  bool is_connected_{false};

  std::atomic_bool waiting_for_frontend_{false};
  std::atomic_bool waiting_for_resume_{false};
  std::atomic_bool running_nested_loop_{false};

  std::unique_ptr<v8_inspector::V8Inspector> inspector_;
  std::unique_ptr<v8_inspector::V8InspectorSession> session_;
};

}  // namespace svm
