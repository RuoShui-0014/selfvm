#pragma once

#include <atomic>

namespace base {

template <typename T>
class Waiter {
 public:
  Waiter() = default;
  ~Waiter() = default;

  void Wait() const {
    while (!finished_.test(std::memory_order_acquire)) {
    }
  }

  T& GetValue() { return value_; }

  T& WaitFor() {
    while (!finished_.test(std::memory_order_acquire)) {
    }
    return value_;
  }

  void SetValue(const T& value) {
    value_ = value;
    finished_.test_and_set(std::memory_order_relaxed);
  }

  bool IsFinished() const { return finished_.test(std::memory_order_acquire); }
  void Notify() { finished_.test_and_set(std::memory_order_relaxed); }

 private:
  std::atomic_flag finished_;
  T value_;
};

template <>
class Waiter<void> {
 public:
  Waiter() = default;
  ~Waiter() = default;

  void Wait() const {
    while (!finished_.test(std::memory_order_acquire)) {
    }
  }

  bool IsFinished() const { return finished_.test(std::memory_order_acquire); }

 private:
  std::atomic_flag finished_;
};

}  // namespace base
