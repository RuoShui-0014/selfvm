#pragma once

#include <condition_variable>
#include <functional>
#include <queue>
#include <semaphore>
#include <thread>
#include <vector>

namespace base {

class ThreadPool {
 public:
  struct Params {
    Params() = default;
    Params(unsigned int max_threads, unsigned int min_threads)
        : max_threads(max_threads), min_threads(min_threads) {}
    unsigned int max_threads;
    unsigned int min_threads;
  };

  ThreadPool();
  ~ThreadPool();

  static ThreadPool* Get();

  void Start(const Params& params = {std::thread::hardware_concurrency(), 1});

  void Execute();

  template <typename Func, typename... Args>
  void PostTask(Func&& task, Args&&... args) {
    auto func =
        std::bind(std::forward<Func>(task), std::forward<Args>(args)...);
    {
      std::lock_guard lock(queue_mutex_);
      queue_.emplace(std::move(func));
    }
    cv_.notify_one();
  }

  void ScheduleTask(std::function<void()>&& task);

 private:
  class ThreadIns {
   public:
    explicit ThreadIns(int id);
    ~ThreadIns();

    void Execute();
    void Stop();
    void Wait();
    int TaskCount() {
      int count = 0;
      {
        std::lock_guard lock(queue_mutex_);
        count = static_cast<int>(queue_.size());
      }
      return count;
    }

    void AddTask(std::function<void()>&& task) {
      {
        std::lock_guard lock(queue_mutex_);
        queue_.emplace(std::move(task));
      }
      cv_.notify_all();
    }

   private:
    std::mutex exec_mutex_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;

    std::thread worker_;
    std::atomic_bool running_;
    std::queue<std::function<void()>> queue_;
    int id_;
  };

  std::mutex exec_mutex_;
  std::mutex queue_mutex_;
  std::condition_variable cv_;

  Params params_;
  std::thread server_;
  std::atomic_bool initialized_{false};
  std::atomic_bool running_;
  std::queue<std::function<void()>> queue_;
  std::vector<std::unique_ptr<ThreadIns>> threads_ins_;

  std::chrono::time_point<std::chrono::steady_clock> start_{
      std::chrono::high_resolution_clock::now()};
};

}  // namespace base
