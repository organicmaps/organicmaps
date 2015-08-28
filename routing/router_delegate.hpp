#pragma once

#include "geometry/point2d.hpp"

#include "base/cancellable.hpp"
#include "base/timer.hpp"

#include "std/function.hpp"

namespace routing
{
class TimeoutCancellable : public my::Cancellable
{
public:
  TimeoutCancellable();

  /// Sets timeout before cancellation. 0 means an infinite timeout.
  void SetTimeout(uint32_t timeoutSec);

  // Cancellable overrides:
  bool IsCancelled() const override;
  void Reset() override;

private:
  my::Timer m_timer;
  uint32_t m_timeoutSec;
};

class RouterDelegate : public TimeoutCancellable
{
public:
  using TProgressCallback = function<void(float)>;
  using TPointCheckCallback = function<void(m2::PointD const &)>;

  RouterDelegate();

  /// Set routing progress. Waits current progress status from 0 to 100.
  void OnProgress(float progress) const { if (!IsCancelled()) m_progressCallback(progress); }
  void OnPointCheck(m2::PointD const & point) const { if (!IsCancelled()) m_pointCallback(point); }

  void SetProgressCallback(TProgressCallback const & progressCallback);
  void SetPointCheckCallback(TPointCheckCallback const & pointCallback);

private:
  TProgressCallback m_progressCallback;
  TPointCheckCallback m_pointCallback;
};

}  //  nomespace routing
