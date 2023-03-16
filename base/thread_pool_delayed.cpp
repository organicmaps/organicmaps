#include "base/thread_pool_delayed.hpp"

#include <array>

using namespace std;

namespace base
{
namespace thread_pool
{
namespace delayed
{
namespace
{
TaskLoop::TaskId MakeNextId(TaskLoop::TaskId id, TaskLoop::TaskId minId, TaskLoop::TaskId maxId)
{
  if (id == maxId)
    return minId;

  return ++id;
}
}  // namespace

ThreadPool::ThreadPool(size_t threadsCount /* = 1 */, Exit e /* = Exit::SkipPending */)
  : m_exit(e)
  , m_immediateLastId(kImmediateMaxId)
  , m_delayedLastId(kDelayedMaxId)
{
  for (size_t i = 0; i < threadsCount; ++i)
    m_threads.emplace_back(threads::SimpleThread(&ThreadPool::ProcessTasks, this));
}

ThreadPool::~ThreadPool()
{
  ShutdownAndJoin();
}

TaskLoop::PushResult ThreadPool::Push(Task && t)
{
  return AddImmediate(move(t));
}

TaskLoop::PushResult ThreadPool::Push(Task const & t)
{
  return AddImmediate(t);
}

TaskLoop::PushResult ThreadPool::PushDelayed(Duration const & delay, Task && t)
{
  return AddDelayed(delay, move(t));
}

TaskLoop::PushResult ThreadPool::PushDelayed(Duration const & delay, Task const & t)
{
  return AddDelayed(delay, t);
}

template <typename T>
TaskLoop::PushResult ThreadPool::AddImmediate(T && task)
{
  return AddTask([&]() {
    auto const newId = MakeNextId(m_immediateLastId, kImmediateMinId, kImmediateMaxId);
    VERIFY(m_immediate.Emplace(newId, forward<T>(task)), ());
    m_immediateLastId = newId;
    return newId;
  });
}

template <typename T>
TaskLoop::PushResult ThreadPool::AddDelayed(Duration const & delay, T && task)
{
  auto const when = Now() + delay;
  return AddTask([&]() {
    auto const newId = MakeNextId(m_delayedLastId, kDelayedMinId, kDelayedMaxId);
    m_delayed.Add(newId, make_shared<DelayedTask>(newId, when, forward<T>(task)));
    m_delayedLastId = newId;
    return newId;
  });
}

template <typename Add>
TaskLoop::PushResult ThreadPool::AddTask(Add && add)
{
  lock_guard<mutex> lk(m_mu);
  if (m_shutdown)
    return {};

  auto const newId = add();
  m_cv.notify_one();
  return {true, newId};
}

void ThreadPool::ProcessTasks()
{
  ImmediateQueue pendingImmediate;
  DelayedQueue pendingDelayed;

  while (true)
  {
    array<Task, QUEUE_TYPE_COUNT> tasks;

    {
      unique_lock<mutex> lk(m_mu);
      if (!m_delayed.IsEmpty())
      {
        // We need to wait until the moment when the earliest delayed
        // task may be executed, given that an immediate task or a
        // delayed task with an earlier execution time may arrive
        // while we are waiting.
        auto const when = m_delayed.GetFirstValue()->m_when;
        m_cv.wait_until(lk, when, [this, when]()
        {
          if (m_shutdown || !m_immediate.IsEmpty() || m_delayed.IsEmpty())
            return true;
          return m_delayed.GetFirstValue()->m_when < when;
        });
      }
      else
      {
        // When there are no delayed tasks in the queue, we need to
        // wait until there is at least one immediate or delayed task.
        m_cv.wait(lk,
                  [this]() { return m_shutdown || !m_immediate.IsEmpty() || !m_delayed.IsEmpty(); });
      }

      if (m_shutdown)
      {
        switch (m_exit)
        {
        case Exit::ExecPending:
          ASSERT(pendingImmediate.IsEmpty(), ());
          m_immediate.Swap(pendingImmediate);

          ASSERT(pendingDelayed.IsEmpty(), ());
          m_delayed.Swap(pendingDelayed);
          break;
        case Exit::SkipPending: break;
        }

        break;
      }

      auto const canExecImmediate = !m_immediate.IsEmpty();
      auto const canExecDelayed =
          !m_delayed.IsEmpty() && Now() >= m_delayed.GetFirstValue()->m_when;

      if (canExecImmediate)
      {
        tasks[QUEUE_TYPE_IMMEDIATE] = move(m_immediate.Front());
        m_immediate.PopFront();
      }

      if (canExecDelayed)
      {
        tasks[QUEUE_TYPE_DELAYED] = move(m_delayed.GetFirstValue()->m_task);
        m_delayed.RemoveValue(m_delayed.GetFirstValue());
      }
    }

    for (auto const & task : tasks)
    {
      if (task)
        task();
    }
  }

  for (; !pendingImmediate.IsEmpty(); pendingImmediate.PopFront())
    pendingImmediate.Front()();

  while (!pendingDelayed.IsEmpty())
  {
    auto const & top = *pendingDelayed.GetFirstValue();
    while (true)
    {
      auto const now = Now();
      if (now >= top.m_when)
        break;
      auto const delay = top.m_when - now;
      this_thread::sleep_for(delay);
    }
    ASSERT(Now() >= top.m_when, ());
    top.m_task();

    pendingDelayed.RemoveValue(pendingDelayed.GetFirstValue());
  }
}

bool ThreadPool::Cancel(TaskId id)
{
  lock_guard<mutex> lk(m_mu);

  if (m_shutdown || id == kNoId)
    return false;

  if (id <= kImmediateMaxId)
  {
    if (m_immediate.Erase(id))
    {
      m_cv.notify_one();
      return true;
    }
  }
  else
  {
    if (m_delayed.RemoveKey(id))
    {
      m_cv.notify_one();
      return true;
    }
  }

  return false;
}

bool ThreadPool::Shutdown(Exit e)
{
  lock_guard<mutex> lk(m_mu);
  if (m_shutdown)
    return false;
  m_shutdown = true;
  m_exit = e;
  m_cv.notify_all();
  return true;
}

void ThreadPool::ShutdownAndJoin()
{
  ASSERT(m_checker.CalledOnOriginalThread(), ());
  Shutdown(m_exit);
  for (auto & thread : m_threads)
  {
    if (thread.joinable())
      thread.join();
  }
  m_threads.clear();
}

bool ThreadPool::IsShutDown()
{
  lock_guard<mutex> lk(m_mu);
  return m_shutdown;
}
}  // namespace delayed
}  // namespace thread_pool
}  // namespace base
