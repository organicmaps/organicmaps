#include "timeout_observer.hpp"

namespace routing
{
namespace
{
void DefaultProgressFn(float /* progress */) {}
void DefaultPointFn(m2::PointD /* point */) {}
}

TimeoutObserver::TimeoutObserver() : m_timeoutSec(0)
{
  m_progressFn = DefaultProgressFn;
  m_pointFn = DefaultPointFn;
}

bool TimeoutObserver::IsCancelled() const
{
  if (m_timeoutSec && m_timer.ElapsedSeconds() > m_timeoutSec)
    return true;
  return Cancellable::IsCancelled();
}

void TimeoutObserver::SetTimeout(uint32_t timeoutSec)
{
  m_timeoutSec = timeoutSec;
  m_timer.Reset();
}

void TimeoutObserver::SetProgressCallback(TProgressCallback const & progressFn)
{
  if (progressFn != nullptr)
    m_progressFn = progressFn;
  else
    m_progressFn = DefaultProgressFn;
}

void TimeoutObserver::SetPointCheckCallback(TPointCheckCallback const & pointFn)
{
  if (pointFn != nullptr)
    m_pointFn = pointFn;
  else
    m_pointFn = DefaultPointFn;
}

}  //  namespace routing
