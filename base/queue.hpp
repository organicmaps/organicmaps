#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

namespace base
{
namespace threads
{
template <typename T>
class DataWrapper
{
public:
  DataWrapper() : m_is_empty(true) {}
  DataWrapper(T const & data) : m_data(data), m_is_empty(false) {}
  DataWrapper(T && data) : m_data(std::move(data)), m_is_empty(false) {}

  T const & Get() const { return m_data; }
  T & Get() { return m_data; }

  bool IsEmpty() const { return m_is_empty; }

private:
  bool m_is_empty;
  T m_data;
};

template <typename T>
class Queue
{
public:
  Queue() = default;
  Queue(Queue const & other)
  {
    std::lock_guard<std::mutex> lk(other.m_mutex);
    m_queue = other.m_queue;
  }

  void Push(T const & value)
  {
    {
      std::lock_guard<std::mutex> lk(m_mutex);
      m_queue.push(value);
    }
    m_cond.notify_one();
  }

  void Push(T && value)
  {
    {
      std::lock_guard<std::mutex> lk(m_mutex);
      m_queue.push(std::move(value));
    }
    m_cond.notify_one();
  }

  void WaitAndPop(T & value)
  {
    std::unique_lock<std::mutex> lk(m_mutex);
    m_cond.wait(lk, [this]{ return !m_queue.empty(); });
    value = m_queue.front();
    m_queue.pop();
  }

  bool TryPop(T & value)
  {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (m_queue.empty())
      return false;

    value = m_queue.front();
    m_queue.pop();
    return true;

  }

  bool Empty() const
  {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_queue.empty();
  }

private:
  mutable std::mutex m_mutex;
  std::queue<T> m_queue;
  std::condition_variable m_cond;
};
}  // namespace threads
}  // namespace base
