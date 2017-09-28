#include "base/worker_thread.hpp"

using namespace std;

namespace base
{
WorkerThread::WorkerThread()
{
  m_thread = threads::SimpleThread(&WorkerThread::ProcessTasks, this);
}

WorkerThread::~WorkerThread()
{
  ASSERT(m_checker.CalledOnOriginalThread(), ());
  Shutdown(Exit::SkipPending);
  m_thread.join();
}

bool WorkerThread::Push(Task && t)
{
  return TouchQueues([&]() { m_immediate.emplace(move(t)); });
}

bool WorkerThread::Push(Task const & t)
{
  return TouchQueues([&]() { m_immediate.emplace(t); });
}

bool WorkerThread::PushDelayed(Duration const & delay, Task && t)
{
  // NOTE: this code depends on the fact that steady_clock is the same
  // for different threads.
  auto const when = Now() + delay;
  return TouchQueues([&]() { m_delayed.emplace(when, move(t)); });
}

bool WorkerThread::PushDelayed(Duration const & delay, Task const & t)
{
  auto const when = Now() + delay;
  return TouchQueues([&]() { m_delayed.emplace(when, t); });
}

void WorkerThread::ProcessTasks()
{
  ImmediateQueue pendingImmediate;
  DelayedQueue pendingDelayed;

  while (true)
  {
    Task task;

    {
      unique_lock<mutex> lk(m_mu);
      if (!m_delayed.empty())
      {
        // We need to wait for the moment when the delayed task must
        // be executed, but may be interrupted earlier, in case of
        // immediate task or another delayed task that must be
        // executed earlier.
        auto const when = m_delayed.top().m_when;
        m_cv.wait_until(lk, when, [this, when]() {
          return m_shutdown || !m_immediate.empty() || m_delayed.top().m_when < when;
        });
      }
      else
      {
        // When there is no delayed tasks in the queue, we need to
        // wait until there is at least one immediate task or delayed
        // task.
        m_cv.wait(lk,
                  [this]() { return m_shutdown || !m_immediate.empty() || !m_delayed.empty(); });
      }

      if (m_shutdown)
      {
        switch (m_exit)
        {
        case Exit::ExecPending:
          ASSERT(pendingImmediate.empty(), ());
          m_immediate.swap(pendingImmediate);

          ASSERT(pendingDelayed.empty(), ());
          m_delayed.swap(pendingDelayed);
          break;
        case Exit::SkipPending: break;
        }

        break;
      }

      auto const canExecImmediate = !m_immediate.empty();
      auto const canExecDelayed = !m_delayed.empty() && Now() >= m_delayed.top().m_when;

      if (!canExecImmediate && !canExecDelayed)
        continue;

      ASSERT(canExecImmediate || canExecDelayed, ());
      bool execImmediate = canExecImmediate;
      bool execDelayed = canExecDelayed;

      if (canExecImmediate && canExecDelayed)
      {
        // Tasks are executed in the Round-Robin order to prevent
        // bias.
        execImmediate = m_lastQueue == QueueType::Delayed;
        execDelayed = m_lastQueue == QueueType::Immediate;
      }

      if (execImmediate)
      {
        task = move(m_immediate.front());
        m_immediate.pop();
        m_lastQueue = QueueType::Immediate;
      }
      else
      {
        ASSERT(execDelayed, ());
        task = move(m_delayed.top().m_task);
        m_delayed.pop();
        m_lastQueue = QueueType::Delayed;
      }
    }

    if (task)
      task();
  }

  for (; !pendingImmediate.empty(); pendingImmediate.pop())
    pendingImmediate.front()();

  for (; !pendingDelayed.empty(); pendingDelayed.pop())
  {
    auto const & top = pendingDelayed.top();
    while(true)
    {
      auto const now = Now();
      if (now >= top.m_when)
        break;
      auto const delay = top.m_when - now;
      this_thread::sleep_for(delay);
    }
    ASSERT(Now() >= top.m_when, ());
    top.m_task();
  }
}

bool WorkerThread::Shutdown(Exit e)
{
  lock_guard<mutex> lk(m_mu);
  if (m_shutdown)
    return false;
  m_shutdown = true;
  m_exit = e;
  m_cv.notify_one();
  return true;
}
}  // namespace base
