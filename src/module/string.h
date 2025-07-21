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
  static constexpr T table_;
};

}  // namespace svm
