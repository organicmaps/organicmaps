#include "deferred_task.hpp"

namespace my
{
DeferredTask::DeferredTask(TDuration const & duration) : m_duration(duration)
{
  m_thread = thread([this]
  {
    unique_lock<mutex> l(m_mutex);
    while (!m_terminate)
    {
      if (!m_fn)
      {
        m_cv.wait(l);
        continue;
      }

      if (m_cv.wait_for(l, m_duration) != cv_status::timeout)
        continue;

      auto fn = move(m_fn);
      m_fn = nullptr;

      if (fn)
      {
        l.unlock();
        fn();
        l.lock();
      }
    }
  });
}

DeferredTask::~DeferredTask()
{
  {
    unique_lock<mutex> l(m_mutex);
    m_terminate = true;
  }
  m_cv.notify_one();
  m_thread.join();
}

void DeferredTask::Drop()
{
  {
    unique_lock<mutex> l(m_mutex);
    m_fn = nullptr;
  }
  m_cv.notify_one();
}
}  // namespace my
