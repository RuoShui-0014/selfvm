//
// Created by ruoshui on 25-7-11.
//

#pragma once

#include <cppgc/member.h>

#include <optional>

#include "../isolate/script_wrappable.h"
#include "../utils/utils.h"
#include "../vendor/v8-inspector.h"

namespace svm {

class IsolateHolder;
class IsolateHandle;
class ContextHandle;
class SessionHandle;
class InspectorChannel;

class InspectorAgent : public v8_inspector::V8InspectorClient {
 public:
  explicit InspectorAgent(SessionHandle* session_handle, v8::Isolate* isolate);
  ~InspectorAgent() override;

  void connectInspector();
  void addContext(v8::Local<v8::Context> context) const;
  void dispatchMessage(std::string message);
  void dispose();

  /* v8_inspector::V8InspectorClient override */
  void runMessageLoopOnPause(int context_group_id) override;
  void quitMessageLoopOnPause() override;
  void runIfWaitingForDebugger(int context_group_id) override;

 private:
  cppgc::WeakMember<SessionHandle> session_handle_;
  v8::Isolate* isolate_{};

  std::atomic_bool waiting_for_frontend_{false};
  std::atomic_bool waiting_for_resume_{false};
  std::atomic_bool running_nested_loop_{false};

  std::unique_ptr<v8_inspector::V8Inspector> inspector_;
  std::unique_ptr<InspectorChannel> channel_;
  std::unique_ptr<v8_inspector::V8InspectorSession> session_;
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

  IsolateHandle* GetIsolateHandle() const;
  void AddContext(v8::Local<v8::Context> context) const;
  void Release();

  /* js interface */
  void DispatchInspectorMessage(std::string message);
  void AddContext(ContextHandle* context_handle);
  void Dispose();

  std::optional<RemoteHandle<v8::Function>> on_response_;
  std::optional<RemoteHandle<v8::Function>> on_notification_;

  void Trace(cppgc::Visitor* visitor) const override;

 private:
  friend class InspectorAgent;
  friend class InspectorChannel;

  cppgc::Member<IsolateHandle> isolate_handle_;
  std::shared_ptr<IsolateHolder> isolate_holder_;
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
