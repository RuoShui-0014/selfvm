#include "logger.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <format>
#include <syncstream>

namespace base {

Logger& Logger::Get() {
  static Logger logger;
  return logger;
}

void Logger::Initialize(std::string file, Level level) {
  Logger& logger = Logger::Get();
  logger.file_.open(file, std::ios::out | std::ios::app);
  logger.level_ = level;

  logger.thread_ = std::thread([]() {
#ifdef _WIN32
    SetThreadDescription(GetCurrentThread(), L"logger server");
#endif

    Logger& logger = Logger::Get();
    while (logger.running_.load()) {
      {
        std::unique_lock lock(logger.mutex_);
        logger.cv_.wait(lock, [&logger]() {
          std::lock_guard lock(logger.mutex_);
          return !logger.queue_.empty() || !logger.running_.load();
        });
      }

      WriteLog();
    }

    WriteLog();
  });
}

void Logger::Log(Level level,
                 std::string message,
                 const std::source_location& location,
                 NowTime time) {
  Logger& logger = Logger::Get();
  if (level < logger.level_) {
    return;
  }

  {
    std::unique_lock lock(logger.mutex_);
    logger.queue_.emplace(level, std::move(message), location, std::move(time));
  }
  logger.cv_.notify_one();
}

Logger::~Logger() {
  Logger& logger = Logger::Get();
  logger.running_.store(false);
  logger.cv_.notify_one();

  if (logger.thread_.joinable()) {
    logger.thread_.join();
  }

  logger.file_.flush();
  logger.file_.close();
}

void Logger::WriteLog() {
  static const char* leval_string[]{"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

  Logger& logger = Logger::Get();
  std::queue<Info> queue;
  {
    std::lock_guard lock(logger.mutex_);
    queue.swap(logger.queue_);
  }
  while (!queue.empty()) {
    Info info = std::move(queue.front());
    queue.pop();
    logger.file_ << std::format(
                        "{:%Y-%m-%d %H:%M:%S} |{: <6}| {} | {}:{}:{}",
                        info.time, leval_string[static_cast<int>(info.level)],
                        info.message, info.location.file_name(),
                        info.location.line(), info.location.function_name())
                 << std::endl;
  }
}

Logger::Logger() : level_(Level::kInfo) {}

}  // namespace base
