#include "routing/router_delegate.hpp"

#include <chrono>

namespace routing
{
namespace
{
void DefaultProgressFn(float /* progress */) {}
void DefaultPointFn(ms::LatLon const & /* point */) {}
}  //  namespace

RouterDelegate::RouterDelegate()
{
  m_progressCallback = DefaultProgressFn;
  m_pointCallback = DefaultPointFn;
}

void RouterDelegate::OnProgress(float progress) const
{
  if (!m_cancellable.IsCancelled())
    m_progressCallback(progress);
}

void RouterDelegate::OnPointCheck(ms::LatLon const & point) const
{
  if (!m_cancellable.IsCancelled())
    m_pointCallback(point);
}

void RouterDelegate::SetProgressCallback(ProgressCallback const & progressCallback)
{
  m_progressCallback = progressCallback ? progressCallback : DefaultProgressFn;
}

void RouterDelegate::SetPointCheckCallback(PointCheckCallback const & pointCallback)
{
  m_pointCallback = pointCallback ? pointCallback : DefaultPointFn;
}

void RouterDelegate::SetTimeout(uint32_t timeoutSec)
{
  if (timeoutSec == kNoTimeout)
    return;

  std::chrono::steady_clock::duration const timeout = std::chrono::seconds(timeoutSec);
  m_cancellable.SetDeadline(std::chrono::steady_clock::now() + timeout);
}
}  //  namespace routing
