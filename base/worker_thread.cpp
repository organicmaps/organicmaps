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

void WorkerThread::ProcessTasks()
{
  queue<Task> pending;

  while (true)
  {
    Task task;

    {
      unique_lock<mutex> lk(m_mu);
      m_cv.wait(lk, [this]() { return m_shutdown || !m_queue.empty(); });

      if (m_shutdown)
      {
        switch (m_exit)
        {
        case Exit::ExecPending:
          CHECK(pending.empty(), ());
          m_queue.swap(pending);
          break;
        case Exit::SkipPending: break;
        }
        break;
      }

      CHECK(!m_queue.empty(), ());
      task = move(m_queue.front());
      m_queue.pop();
    }

    task();
  }

  while (!pending.empty())
  {
    pending.front()();
    pending.pop();
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
