#include "logger.h"

#include <filesystem>

#ifdef OS_WIN
#include <Windows.h>
#endif

#include <format>
#include <syncstream>

namespace base {

Logger& Logger::Instance() {
  static Logger logger;
  return logger;
}

void Logger::Initialize(std::string file, Level level) {
  Logger& logger{Instance()};

  std::filesystem::create_directories(
      std::filesystem::path{file}.parent_path());
  logger.file_.open(file, std::ios::out | std::ios::app);
  logger.level_ = level;

  logger.thread_ = std::thread([]() {
#ifdef OS_WIN
    SetThreadDescription(GetCurrentThread(), L"logger server");
#endif

    Logger& logger{Instance()};
    while (logger.running_.load(std::memory_order_acquire)) {
      {
        std::unique_lock lock{logger.mutex_};
        logger.cv_.wait(lock, [&logger]() {
          return !logger.queue_.empty() || !logger.running_.load();
        });
      }

      Write();
    }

    Write();
  });
}

void Logger::Log(Level level,
                 std::string message,
                 const std::source_location& location,
                 NowTime time) {
  Logger& logger{Instance()};
  if (level < logger.level_) {
    return;
  }
  {
    std::unique_lock lock{logger.mutex_};
    logger.queue_.emplace(level, std::move(message), location, std::move(time));
  }
  logger.cv_.notify_one();
}

Logger::~Logger() {
  Logger& logger{Instance()};
  logger.running_.store(false, std::memory_order_release);
  logger.cv_.notify_one();

  if (logger.thread_.joinable()) {
    logger.thread_.join();
  }

  logger.file_.flush();
  logger.file_.close();
}

void Logger::Write() {
  static constexpr const char* leval_string[]{"DEBUG", "INFO", "WARN", "ERROR",
                                              "FATAL"};

  Logger& logger{Instance()};
  std::queue<Info> queue;
  {
    std::lock_guard lock{logger.mutex_};
    queue.swap(logger.queue_);
  }
  while (!queue.empty()) {
    Info info{std::move(queue.front())};
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
