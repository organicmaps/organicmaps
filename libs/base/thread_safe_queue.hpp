#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

namespace threads
{
template <typename T>
class ThreadSafeQueue
{
public:
  ThreadSafeQueue() = default;
  ThreadSafeQueue(ThreadSafeQueue const & other)
  {
    std::lock_guard<std::mutex> lk(other.m_mutex);
    m_queue = other.m_queue;
  }

  void Push(T const & value)
  {
    {
      std::lock_guard lk(m_mutex);
      m_queue.push(value);
    }
    m_cond.notify_one();
  }

  void Push(T && value)
  {
    {
      std::lock_guard lk(m_mutex);
      m_queue.push(std::move(value));
    }
    m_cond.notify_one();
  }

  void WaitAndPop(T & value)
  {
    std::unique_lock lk(m_mutex);
    m_cond.wait(lk, [this] { return !m_queue.empty(); });
    value = std::move(m_queue.front());
    m_queue.pop();
  }

  bool TryPop(T & value)
  {
    std::lock_guard lk(m_mutex);
    if (m_queue.empty())
      return false;

    value = std::move(m_queue.front());
    m_queue.pop();
    return true;
  }

  bool Empty() const
  {
    std::lock_guard lk(m_mutex);
    return m_queue.empty();
  }

  size_t Size() const
  {
    std::lock_guard lk(m_mutex);
    return m_queue.size();
  }

private:
  mutable std::mutex m_mutex;
  std::queue<T> m_queue;
  std::condition_variable m_cond;
};
}  // namespace threads
