#include "platform_delegate.h"

#include <map>

namespace svm {
namespace {
PlatformDelegate delegate;
}

void PlatformDelegate::InitializeDelegate() {
  auto context{v8::Isolate::GetCurrent()->GetCurrentContext()};
  auto* node_platform{
      node::GetMultiIsolatePlatform(node::GetCurrentEnvironment(context))};
  delegate = PlatformDelegate{node_platform};
}

node::MultiIsolatePlatform* PlatformDelegate::GetNodePlatform() {
  return delegate.node_platform;
}

void PlatformDelegate::RegisterIsolate(
    v8::Isolate* isolate,
    node::IsolatePlatformDelegate* isolate_delegate) {
  delegate.node_platform->RegisterIsolate(isolate, isolate_delegate);
}

void PlatformDelegate::RegisterIsolate(v8::Isolate* isolate, uv_loop_t* loop) {
  delegate.node_platform->RegisterIsolate(isolate, loop);
}

void PlatformDelegate::UnregisterIsolate(v8::Isolate* isolate) {
  delegate.node_platform->UnregisterIsolate(isolate);
}
}  // namespace svm
