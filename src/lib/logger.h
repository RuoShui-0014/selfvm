#pragma once
#include <atomic>
#include <chrono>
#include <format>
#include <fstream>
#include <mutex>
#include <queue>
#include <source_location>
#include <string>
#include <thread>

namespace base {

class Logger {
 public:
  enum class Level { kDebug, kInfo, kWarn, kError, kFatal };

  static Logger& Get();
  static void Initialize(std::string file, Level level);

  using NowTime = std::chrono::system_clock::time_point;
  static void Log(
      Level level,
      std::string message,
      const std::source_location& location = std::source_location::current(),
      NowTime time = std::chrono::system_clock::now());

  ~Logger();

 private:
  struct Info {
    Level level;
    std::string message;
    std::source_location location;
    NowTime time;
  };
  static void WriteLog();
  Logger();

  std::mutex mutex_;
  std::condition_variable cv_;

  Level level_;
  std::atomic_bool running_{true};
  std::ofstream file_;
  std::thread thread_;
  std::queue<Info> queue_;
};

#define LOG_DEBUG(msg) base::Logger::Log(base::Logger::Level::kDebug, (msg));
#define LOG_INFO(msg) base::Logger::Log(base::Logger::Level::kInfo, (msg));
#define LOG_WARN(msg) base::Logger::Log(base::Logger::Level::kWarn, (msg));
#define LOG_ERROR(msg) base::Logger::Log(base::Logger::Level::kError, (msg));
#define LOG_FATAL(msg) base::Logger::Log(base::Logger::Level::kFatal, (msg));

}  // namespace base
