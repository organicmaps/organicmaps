#include "base/deferred_task.hpp"

namespace base
{
DeferredTask::DeferredTask(Duration const & duration) : m_duration(duration)
{
  m_thread = threads::SimpleThread([this]
  {
    std::unique_lock l(m_mutex);
    while (!m_terminate)
    {
      if (!m_fn)
      {
        m_cv.wait(l);
        continue;
      }

      if (m_cv.wait_for(l, m_duration) != std::cv_status::timeout || !m_fn)
        continue;

      auto fn = std::move(m_fn);
      m_fn = nullptr;

      l.unlock();
      fn();
      l.lock();
    }
  });
}

DeferredTask::~DeferredTask()
{
  {
    std::unique_lock l(m_mutex);
    m_terminate = true;
  }
  m_cv.notify_one();
  m_thread.join();
}

void DeferredTask::Drop()
{
  {
    std::unique_lock l(m_mutex);
    m_fn = nullptr;
  }
  m_cv.notify_one();
}
}  // namespace base
