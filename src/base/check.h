#pragma once

#include "config.h"

#ifdef DEBUG_FLAG
#include <cstdlib>
#include <iostream>//
#define CHECK(condition, message)                                     \
  do {                                                                \
    if (!(condition)) {                                               \
      std::cerr << "Debug assertion failed: " << #condition           \
                << "\nMessage: " << message << "\nFile: " << __FILE__ \
                << "\nLine: " << __LINE__ << std::endl;               \
      std::abort();                                                   \
    }                                                                 \
  } while (0)

#define CHECK_EQ(condition_1, condition_2, message)                                     \
  do {                                                                                  \
    if (!((condition_1) == (condition_2)) {                                           \
      std::cerr << "Debug assertion failed: " << #condition_1 << "==" << #condition_2 \
                << "\nMessage: " << message << "\nFile: " << __FILE__                 \
                << "\nLine: " << __LINE__ << std::endl;                               \
      std::abort();                                                                   \
    }                                                                                   \
  } while (0)

#define CHECK_NE(condition_1, condition_2, message)                                     \
  do {                                                                                  \
    if (!((condition_1) != (condition_2)) {                                           \
      std::cerr << "Debug assertion failed: " << #condition_1 << "!=" << #condition_2 \
                << "\nMessage: " << message << "\nFile: " << __FILE__                 \
                << "\nLine: " << __LINE__ << std::endl;                               \
      std::abort();                                                                   \
    }                                                                                   \
  } while (0)

#else
#define CHECK(condition, message) ((void)0)
#define CHECK_EQ(condition_1, condition_2, message) ((void)0)
#endif
