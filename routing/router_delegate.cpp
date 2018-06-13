#include "routing/router_delegate.hpp"

namespace routing
{
namespace
{
void DefaultProgressFn(float /* progress */) {}
void DefaultPointFn(m2::PointD const & /* point */) {}
} //  namespace

RouterDelegate::RouterDelegate()
{
  m_progressCallback = DefaultProgressFn;
  m_pointCallback = DefaultPointFn;
}

void RouterDelegate::SetProgressCallback(ProgressCallback const & progressCallback)
{
  m_progressCallback = progressCallback ? progressCallback : DefaultProgressFn;
}

void RouterDelegate::SetPointCheckCallback(PointCheckCallback const & pointCallback)
{
  m_pointCallback = pointCallback ? pointCallback : DefaultPointFn;
}

void RouterDelegate::OnProgress(float progress) const
{
  lock_guard<mutex> l(m_guard);
  if (!IsCancelled())
    m_progressCallback(progress);
}

void RouterDelegate::Reset()
{
  lock_guard<mutex> l(m_guard);
  TimeoutCancellable::Reset();
}

void RouterDelegate::OnPointCheck(m2::PointD const & point) const
{
  lock_guard<mutex> l(m_guard);
  if (!IsCancelled())
    m_pointCallback(point);
}

TimeoutCancellable::TimeoutCancellable() : m_timeoutSec(0)
{
}

bool TimeoutCancellable::IsCancelled() const
{
  if (m_timeoutSec && m_timer.ElapsedSeconds() > m_timeoutSec)
    return true;
  return Cancellable::IsCancelled();
}

void TimeoutCancellable::Reset()
{
  m_timeoutSec = 0;
  Cancellable::Reset();
}

void TimeoutCancellable::SetTimeout(uint32_t timeoutSec)
{
  m_timeoutSec = timeoutSec;
  m_timer.Reset();
}

}  //  namespace routing
