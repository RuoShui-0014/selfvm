#pragma once

#include <node.h>
#include <uv.h>

namespace svm {
class PlatformDelegate final {
 public:
  PlatformDelegate() = delete;
  ~PlatformDelegate() = delete;
  PlatformDelegate(const PlatformDelegate&) = delete;
  PlatformDelegate(PlatformDelegate&&) = delete;
  auto operator=(const PlatformDelegate&) = delete;
  auto operator=(PlatformDelegate&& delegate) noexcept = delete;

  static void InitializeDelegate();
  static node::MultiIsolatePlatform* GetNodePlatform();
  static void RegisterIsolate(v8::Isolate* isolate,
                              node::IsolatePlatformDelegate* isolate_delegate);
  static void RegisterIsolate(v8::Isolate* isolate, uv_loop_t* loop);
  static void UnregisterIsolate(v8::Isolate* isolate);
};
}  // namespace svm
