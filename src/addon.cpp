#include "isolate/per_isolate_data.h"
#include "isolate/platform_delegate.h"
#include "isolate/scheduler.h"
#include "module/isolate_handle.h"
#include "utils/utils.h"

namespace svm {

void NodeGcOperationCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  info.GetIsolate()->LowMemoryNotification();
}

PerIsolateData* g_per_isolate_data{nullptr};

void Initialize(v8::Local<v8::Object> exports) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  // init nodejs env
  PlatformDelegate::InitializeDelegate();
  Scheduler::RegisterIsolate(isolate, node::GetCurrentEventLoop(isolate));
  g_per_isolate_data = new PerIsolateData(isolate);

  // can release nodejs isolate memory
  v8::Local<v8::String> str = toString(isolate, "gc");
  v8::Local<v8::FunctionTemplate> tmpl =
      v8::FunctionTemplate::New(isolate, NodeGcOperationCallback, {}, {}, 0,
                                v8::ConstructorBehavior::kThrow);
  tmpl->SetClassName(str);
  exports->Set(context, str, tmpl->GetFunction(context).ToLocalChecked());

  // IsolateHandle construct
  exports->Set(context, toString(isolate, "Isolate"),
               NewFunction<V8IsolateHandle>(isolate, context));

  node::AddEnvironmentCleanupHook(
      isolate, [](void* arg) { delete static_cast<PerIsolateData*>(arg); },
      g_per_isolate_data);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)
}  // namespace svm
