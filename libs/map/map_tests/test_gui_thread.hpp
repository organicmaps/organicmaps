#pragma once

#include "testing/testing.hpp"

#include "base/task_loop.hpp"
#include "base/thread_safe_queue.hpp"
#include "base/waiter.hpp"

#include <chrono>

namespace map_tests
{
// Pump only this fixture's tasks, not callbacks left by earlier Framework instances.
class TestGuiThread final : public base::TaskLoop
{
public:
  PushResult Push(Task && task) override
  {
    m_tasks.Push(std::move(task));
    m_tasksReady.Notify();
    return {true, kNoId};
  }
  PushResult Push(Task const & task) override { return Push(Task(task)); }

  template <typename Predicate>
  void RunUntil(Predicate && done)
  {
    auto const deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (!done())
    {
      TEST(std::chrono::steady_clock::now() < deadline, ("GUI callback timed out"));
      Task task;
      while (!m_tasks.TryPop(task))
      {
        TEST(m_tasksReady.Wait(deadline - std::chrono::steady_clock::now()) != base::Waiter::Result::Timeout,
             ("GUI callback timed out"));
        m_tasksReady.Reset();
      }
      task();
    }
  }

private:
  threads::ThreadSafeQueue<Task> m_tasks;
  base::Waiter m_tasksReady;
};
}  // namespace map_tests
