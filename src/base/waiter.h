#pragma once

#include <atomic>

namespace base {

template <typename T>
class LazyWaiter {
 public:
  LazyWaiter() = default;
  ~LazyWaiter() = default;

  void Wait() const { finished_.wait(false); }

  T& GetValue() { return value_; }

  T& WaitFor() {
    finished_.wait(false);
    return value_;
  }

  void SetValue(const T& value) {
    value_ = value;
    Notify();
  }

  void SetValue(T&& value) {
    value_ = std::move(value);
    Notify();
  }

  bool IsFinished() const { return finished_.load(); }
  void Notify() {
    finished_.store(true);
    finished_.notify_one();
  }

 private:
  std::atomic_bool finished_{false};
  T value_;
};

template <>
class LazyWaiter<void> {
 public:
  LazyWaiter() = default;
  ~LazyWaiter() = default;

  void Wait() const { finished_.wait(false, std::memory_order_acq_rel); }
  void Notify() {
    finished_.store(true, std::memory_order_relaxed);
    finished_.notify_one();
  }
  bool IsFinished() const { return finished_.load(std::memory_order_acquire); }

 private:
  std::atomic_bool finished_;
};

template <typename T>
class SpinWaiter {
 public:
  SpinWaiter() = default;
  ~SpinWaiter() = default;

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
    Notify();
  }

  bool IsFinished() const { return finished_.test(std::memory_order_acquire); }
  void Notify() { finished_.test_and_set(std::memory_order_relaxed); }

 private:
  std::atomic_flag finished_;
  T value_;
};

template <>
class SpinWaiter<void> {
 public:
  SpinWaiter() = default;
  ~SpinWaiter() = default;

  void Wait() const {
    while (!finished_.test(std::memory_order_acquire)) {
    }
  }

  bool IsFinished() const { return finished_.test(std::memory_order_acquire); }
  void Notify() { finished_.test_and_set(std::memory_order_relaxed); }

 private:
  std::atomic_flag finished_;
};

}  // namespace base
