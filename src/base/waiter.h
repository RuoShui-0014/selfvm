#pragma once

#include <atomic>

namespace base {

template <typename T>
class Waiter {
 public:
  Waiter() = default;
  ~Waiter() = default;

  void Wait() const {
    while (!finished_.load(std::memory_order_acquire)) {
    }
  }

  T& GetValue() { return value_; }

  T& WaitFor() {
    while (!finished_.load(std::memory_order_acquire)) {
    }
    return value_;
  }

  void SetValue(const T& value) {
    value_ = value;
    finished_.store(true, std::memory_order_relaxed);
  }

  bool IsFinished() const { return finished_.load(std::memory_order_acquire); }

 private:
  std::atomic_bool finished_{false};
  T value_;
};

template <>
class Waiter<void> {
 public:
  Waiter() = default;
  ~Waiter() = default;

  void Wait() const {
    while (!finished_.load(std::memory_order_acquire)) {
    }
  }

  bool IsFinished() const { return finished_.load(std::memory_order_acquire); }

 private:
  std::atomic_bool finished_{false};
};

}  // namespace base
