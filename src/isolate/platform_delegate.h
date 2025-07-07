#pragma once

#include <node.h>
#include <uv.h>

namespace svm {
class PlatformDelegate {
 public:
  explicit PlatformDelegate(node::MultiIsolatePlatform* node_platform)
      : node_platform{node_platform} {}

  PlatformDelegate() = default;
  ~PlatformDelegate() = default;

  PlatformDelegate(const PlatformDelegate&) = delete;
  PlatformDelegate(PlatformDelegate&&) = delete;
  auto operator=(const PlatformDelegate&) = delete;

  auto operator=(PlatformDelegate&& delegate) noexcept -> PlatformDelegate& {
    node_platform = std::exchange(delegate.node_platform, nullptr);
    return *this;
  }

  static void InitializeDelegate();

  static node::MultiIsolatePlatform* GetNodePlatform();

  static void RegisterIsolate(v8::Isolate* isolate,
                              node::IsolatePlatformDelegate* isolate_delegate);

  static void RegisterIsolate(v8::Isolate* isolate,
                              uv_loop_t* loop);

  static void UnregisterIsolate(v8::Isolate* isolate);

  node::MultiIsolatePlatform* node_platform = nullptr;
};
}  // namespace svm
