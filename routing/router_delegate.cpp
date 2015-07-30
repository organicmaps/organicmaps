#include "router_delegate.hpp"

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

void RouterDelegate::SetProgressCallback(TProgressCallback const & progressCallback)
{
  m_progressCallback = (progressCallback) ? progressCallback : DefaultProgressFn;
}

void RouterDelegate::SetPointCheckCallback(TPointCheckCallback const & pointCallback)
{
  m_pointCallback = (pointCallback) ? pointCallback : DefaultPointFn;
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
