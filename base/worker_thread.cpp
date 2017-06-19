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
  unique_lock<mutex> lk(m_mu, defer_lock);

  while (true)
  {
    Task task;

    {
      lk.lock();
      m_cv.wait(lk, [this]() { return m_shutdown || !m_queue.empty(); });

      if (m_shutdown)
      {
        switch (m_exit)
        {
        case Exit::ExecPending:
        {
          while (!m_queue.empty())
          {
            m_queue.front()();
            m_queue.pop();
          }
          break;
        }
        case Exit::SkipPending: break;
        }

        break;
      }

      CHECK(!m_queue.empty(), ());
      task = move(m_queue.front());
      m_queue.pop();
      lk.unlock();
    }

    task();
  }
}

void WorkerThread::Shutdown(Exit e)
{
  ASSERT(m_checker.CalledOnOriginalThread(), ());

  if (m_shutdown)
    return;

  CHECK(!m_shutdown, ());
  lock_guard<mutex> lk(m_mu);
  m_shutdown = true;
  m_exit = e;
  m_cv.notify_one();
}
}  // namespace base
