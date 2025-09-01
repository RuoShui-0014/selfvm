#pragma once

#if defined(OS_WIN)

#if defined(BUILDING_SHARED)
#define BASE_EXPORT __declspec(dllexport)
#elif defined(USING_SHARED)
#define BASE_EXPORT __declspec(dllimport)
#else
#define BASE_EXPORT
#endif

#else

#define BASE_EXPORT __attribute__((visibility("default")))

#endif

#if defined(DEBUG) || defined(_DEBUG) || defined(DEBUG_)
#define DEBUG_FLAG
#define DEBUG_CODE_FLAG 1
#else
#define DEBUG_CODE_FLAG 0
#endif

#define DEBUG_CODE(x)      \
  do {                     \
    if (DEBUG_CODE_FLAG) { \
      x                    \
    }                      \
  } while (0);

#define DEBUG_RELEASE_CODE(x, y)     \
  do {                               \
    if constexpr (DEBUG_CODE_FLAG) { \
      x                              \
    } else {                         \
      y                              \
    }                                \
  } while (0);
