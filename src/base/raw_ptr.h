#pragma once

namespace base {

template <typename T>
class raw_ptr {
 public:
  raw_ptr() = default;
  explicit raw_ptr(T* ptr) : ptr_{ptr} {}
  ~raw_ptr() { ptr_ = nullptr; }

  void operator=(T* ptr) { ptr_ = ptr; }

  raw_ptr(const raw_ptr&) = delete;
  raw_ptr(raw_ptr&&) = delete;
  raw_ptr& operator=(const raw_ptr&) = delete;

  T* get() const { return ptr_; }
  void reset() { ptr_ = nullptr; }
  T& operator*() const { return *ptr_; }
  T* operator->() const { return ptr_; }
  explicit operator bool() const { return ptr_ == nullptr; }

 private:
  T* ptr_{nullptr};
};

}  // namespace base
