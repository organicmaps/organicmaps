#include "router_delegate.hpp"

namespace routing
{
namespace
{
void DefaultProgressFn(float /* progress */) {}
void DefaultPointFn(m2::PointD /* point */) {}
} //  namespace

RouterDelegate::RouterDelegate()
{
  m_progressFn = DefaultProgressFn;
  m_pointFn = DefaultPointFn;
}

void RouterDelegate::SetProgressCallback(TProgressCallback const & progressFn)
{
  if (progressFn != nullptr)
    m_progressFn = progressFn;
  else
    m_progressFn = DefaultProgressFn;
}

void RouterDelegate::SetPointCheckCallback(TPointCheckCallback const & pointFn)
{
  if (pointFn != nullptr)
    m_pointFn = pointFn;
  else
    m_pointFn = DefaultPointFn;
}

bool TimeoutCancellable::IsCancelled() const
{
  if (m_timeoutSec && m_timer.ElapsedSeconds() > m_timeoutSec)
    return true;
  return Cancellable::IsCancelled();
}

void TimeoutCancellable::SetTimeout(uint32_t timeoutSec)
{
  m_timeoutSec = timeoutSec;
  m_timer.Reset();
}

}  //  namespace routing
