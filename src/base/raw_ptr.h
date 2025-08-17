//
// Created by ruoshui on 25-7-25.
//

#pragma once

template <typename T>
class raw_ptr {
 public:
  raw_ptr() = default;
  explicit raw_ptr(T* ptr) : ptr_{ptr} {}
  ~raw_ptr() { ptr_ = nullptr; }

  raw_ptr(const raw_ptr&) = delete;
  raw_ptr(raw_ptr&&) = delete;
  raw_ptr& operator=(const raw_ptr&) = delete;

  T* get() const { return ptr_; }
  T& operator*() const { return *ptr_; }
  T* operator->() const { return ptr_; }
  explicit operator bool() const { return ptr_ == nullptr; }

 private:
  T* ptr_{nullptr};
};
