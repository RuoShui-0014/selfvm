#pragma once

#include <cppgc/member.h>

#include <optional>

#include "isolate/script_wrappable.h"
#include "utils/utils.h"
#include "vendor/v8-inspector.h"

namespace svm {

class IsolateHolder;
class IsolateHandle;
class ContextHandle;

class SessionHandle final : public ScriptWrappable {
 public:
  explicit SessionHandle(IsolateHandle* isolate_handle);
  ~SessionHandle() override;

  IsolateHandle* GetIsolateHandle() const;
  std::shared_ptr<IsolateHolder> GetIsolateHolder() const;

  /* js interface */
  void Connect(int port) const;
  void AddContext(const ContextHandle* context_handle, std::string name);
  void Dispose() const;

  void Trace(cppgc::Visitor* visitor) const override;

 private:
  friend class InspectorAgent;
  friend class InspectorChannel;

  cppgc::Member<IsolateHandle> isolate_handle_;
  std::weak_ptr<IsolateHolder> isolate_holder_;
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
