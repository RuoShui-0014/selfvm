//
// Created by ruoshui on 25-7-11.
//

#pragma once

#include <mutex>
#include <queue>

#include "../isolate/script_wrappable.h"
#include "../node_ex/v8-inspector.h"
#include "../utils/utils.h"
#include "context_handle.h"

namespace svm {

class IsolateHandle;
class SessionHandle;
class InspectorChannel;

class InspectorAgent : public v8_inspector::V8InspectorClient {
 public:
  explicit InspectorAgent(SessionHandle* session_handle, v8::Isolate* isolate);
  ~InspectorAgent() override;

  void connectInspector();
  void addContext(v8::Local<v8::Context> context);
  void dispatchMessage(std::string message);

  /* v8_inspector::V8InspectorClient override */
  void runMessageLoopOnPause(int contextGroupId) override;
  void quitMessageLoopOnPause() override;

 private:
  std::mutex mutex_;
  std::condition_variable cv_;
  std::queue<std::unique_ptr<v8::Task>> tasks_;
  bool paused_{false};

  cppgc::WeakMember<SessionHandle> session_handle_;
  v8::Isolate* isolate_;
  std::unique_ptr<v8_inspector::V8Inspector> inspector_;
  std::unique_ptr<InspectorChannel> channel_;
  std::shared_ptr<v8_inspector::V8InspectorSession> session_;
};

class InspectorChannel : public v8_inspector::V8Inspector::Channel {
 public:
  explicit InspectorChannel(SessionHandle* session_handle);
  ~InspectorChannel() override;

  /* v8_inspector::V8Inspector::Channel override */
  void sendResponse(
      int callId,
      std::unique_ptr<v8_inspector::StringBuffer> message) override;
  void sendNotification(
      std::unique_ptr<v8_inspector::StringBuffer> message) override;
  void flushProtocolNotifications() override;

 private:
  cppgc::WeakMember<SessionHandle> session_handle_;
};

class SessionHandle : public ScriptWrappable {
 public:
  explicit SessionHandle(IsolateHandle* isolate_handle);
  ~SessionHandle() override;

  void PostTaskToPar(std::unique_ptr<v8::Task> task);
  void PostTaskToSel(std::unique_ptr<v8::Task> task);

  /* js interface */
  void DispatchInspectorMessage(std::string message);
  void AddContext(ContextHandle* context_handle);

  std::optional<RemoteHandle<v8::Function>> on_response_;
  std::optional<RemoteHandle<v8::Function>> on_notification_;

 private:
  cppgc::Member<IsolateHandle> isolate_handle_;
  std::unique_ptr<InspectorAgent> inspector_agent_;
};

class V8SessionHandle {
public:
  static constexpr const WrapperTypeInfo* GetWrapperTypeInfo() {
    return &wrapper_type_info_;
  }

  static void InstallInterfaceTemplate(
      v8::Isolate* isolate,
      v8::Local<v8::Template> interface_template);

private:
  friend SessionHandle;
  static const WrapperTypeInfo wrapper_type_info_;
};

}  // namespace svm
