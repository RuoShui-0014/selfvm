#include "thread_pool.h"

#include <Windows.h>

#include <thread>


namespace base {

ThreadPool::ThreadPool() {
  Start({20, 3});
}
ThreadPool::~ThreadPool() {
  running_.store(false);
  if (server_.joinable()) {
    server_.join();
  }

  auto now = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - start_);
}

ThreadPool* ThreadPool::Get() {
  static ThreadPool thread_pool;
  return &thread_pool;
}

void ThreadPool::Start(const Params& params) {
  params_ = params;

  running_.store(true);
  server_ = std::thread(&ThreadPool::Execute, this);

  for (uint32_t i = 0; i < params_.min_threads; i++) {
    threads_ins_.emplace_back(std::make_unique<ThreadIns>(i));
  }
}
void ThreadPool::Execute() {
  SetThreadDescription(GetCurrentThread(), L"thread pool server");
  while (running_.load()) {
    {
      std::unique_lock lock(exec_mutex_);
      cv_.wait_until(
          lock,
          std::chrono::system_clock::now() + std::chrono::milliseconds(10),
          [this]() { return !running_.load(); });
    }

    std::queue<std::function<void()>> tasks;
    {
      std::lock_guard lock(queue_mutex_);
      tasks.swap(queue_);
    }
    while (!tasks.empty()) {
      auto task = std::move(tasks.front());
      tasks.pop();
      ScheduleTask(std::move(task));
    }
  }

  for (auto& ins : threads_ins_) {
    ins->Stop();
  }
  for (auto& ins : threads_ins_) {
    ins->Wait();
  }
}

void ThreadPool::ScheduleTask(std::function<void()>&& task) {
  std::vector<int> data(threads_ins_.size());
  for (int i = 0; i < data.size(); i++) {
    data[i] = threads_ins_[i]->TaskCount();
  }
  auto min_it = std::ranges::min_element(data);
  for (int i = 0; i < data.size(); i++) {
    if (data.at(i) != *min_it) {
      continue;
    }
    threads_ins_[i]->AddTask(std::move(task));
    return;
  }
}

ThreadPool::ThreadIns::ThreadIns(int id) : id_(id) {
  running_.store(true);
  worker_ = std::thread(&ThreadIns::Execute, this);
}

ThreadPool::ThreadIns::~ThreadIns() = default;

void ThreadPool::ThreadIns::Execute() {
  SetThreadDescription(GetCurrentThread(), L"thread pool child");
  while (running_.load() || TaskCount() > 0) {
    {
      std::unique_lock lock(exec_mutex_);
      cv_.wait(lock, [this]() { return TaskCount() || !running_.load(); });
    }

    std::function<void()> task;
    {
      std::lock_guard lock(queue_mutex_);
      if (!queue_.empty()) {
        task = std::move(queue_.front());
        queue_.pop();
      } else {
        continue;
      }
    }
    task();
  }
}

void ThreadPool::ThreadIns::Stop() {
  running_.store(false);
  cv_.notify_all();
}

void ThreadPool::ThreadIns::Wait() {
  worker_.join();
}

}  // namespace base
