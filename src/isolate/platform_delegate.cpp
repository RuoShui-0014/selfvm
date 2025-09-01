#include "isolate/platform_delegate.h"

#include "base/check.h"

namespace svm {

namespace {
node::MultiIsolatePlatform* g_node_platform{nullptr};
}

void PlatformDelegate::InitializeDelegate() {
  const auto context{v8::Isolate::GetCurrent()->GetCurrentContext()};
  auto* node_platform{
      node::GetMultiIsolatePlatform(node::GetCurrentEnvironment(context))};
  g_node_platform = node_platform;
}

node::MultiIsolatePlatform* PlatformDelegate::GetNodePlatform() {
  CHECK(g_node_platform != nullptr, "Platform delegate not initialize.");
  return g_node_platform;
}

void PlatformDelegate::RegisterIsolate(
    v8::Isolate* isolate,
    node::IsolatePlatformDelegate* isolate_delegate) {
  GetNodePlatform()->RegisterIsolate(isolate, isolate_delegate);
}

void PlatformDelegate::RegisterIsolate(v8::Isolate* isolate, uv_loop_t* loop) {
  GetNodePlatform()->RegisterIsolate(isolate, loop);
}

void PlatformDelegate::UnregisterIsolate(v8::Isolate* isolate) {
  GetNodePlatform()->UnregisterIsolate(isolate);
}

}  // namespace svm
