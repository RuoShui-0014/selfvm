//
// Created by ruoshui on 25-7-19.
//

#pragma once
#include "../isolate/script_wrappable.h"
#include "../isolate/wrapper_type_info.h"

namespace svm {

class IsolateHolder;
class IsolateHandle;
class ContextHandle;

using ScriptId = v8::UnboundScript*;

class ScriptHandle : public ScriptWrappable {
 public:
  static ScriptHandle* Create(IsolateHandle* isolate_handle,
                              std::string script,
                              std::string filename);

  explicit ScriptHandle(IsolateHandle* isolate_handle, ScriptId address);
  ~ScriptHandle() override;

  v8::Local<v8::UnboundScript> GetUnboundScript() const;

  /*********************** js interface *************************/
  std::pair<uint8_t*, size_t> Run(ContextHandle* context_handle);
  void Release();

  void Trace(cppgc::Visitor* visitor) const override;

 private:
  cppgc::Member<IsolateHandle> isolate_handle_;
  std::shared_ptr<IsolateHolder> isolate_holder_;
  ScriptId const address_;
};

class V8ScriptHandle {
 public:
  static constexpr const WrapperTypeInfo* GetWrapperTypeInfo() {
    return &wrapper_type_info_;
  }

  static void InstallInterfaceTemplate(
      v8::Isolate* isolate,
      v8::Local<v8::Template> interface_template);

 private:
  friend ScriptHandle;
  static const WrapperTypeInfo wrapper_type_info_;
};

}  // namespace svm
