#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

namespace base
{
namespace threads
{
// DataWrapper functionality is similar to boost::optional. DataWrapper is needed to help send a
// signal to the thread, that there is no more data and it's time to finish the work, i.e. in fact,
// it will be empty only once. I don't want to use boost::optional for this, because it allocates
// memory on the heap, unlike DataWrapper, which allocates it on the stack.
template <typename T>
class DataWrapper
{
public:
  DataWrapper() : m_isEmpty(true) {}
  DataWrapper(T const & data) : m_data(data), m_isEmpty(false) {}
  DataWrapper(T && data) : m_data(std::move(data)), m_isEmpty(false) {}

  T const & Get() const { return m_data; }
  T & Get() { return m_data; }

  bool IsEmpty() const { return m_isEmpty; }

private:
  T m_data;
  bool m_isEmpty;
};

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
    value = std::move(m_queue.front());
    m_queue.pop();
  }

  bool TryPop(T & value)
  {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (m_queue.empty())
      return false;

    value = std::move(m_queue.front());
    m_queue.pop();
    return true;

  }

  bool Empty() const
  {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_queue.empty();
  }

  size_t Size() const
  {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_queue.size();
  }

private:
  mutable std::mutex m_mutex;
  std::queue<T> m_queue;
  std::condition_variable m_cond;
};
}  // namespace threads
}  // namespace base
