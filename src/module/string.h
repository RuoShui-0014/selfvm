//
// Created by ruoshui on 25-7-18.
//

#pragma once

#include <string>

namespace svm {

template <class T>
class StringTable {
 public:
  static constexpr T Get() { return table_; }

 private:
  static T table_;
};

template <class T>
T StringTable<T>::table_{};

}  // namespace svm
