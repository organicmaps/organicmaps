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
    TimeoutCancellable() : m_timeoutSec(0) {}

    /// Sets timeout in seconds to autamaticly calcel. 0 is infinity value.
    void SetTimeout(uint32_t);
    bool IsCancelled() const override;
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
  void OnProgress(float progress) const { m_progressFn(progress); }
  void OnPointCheck(m2::PointD const & point) const { m_pointFn(point); }

  void SetProgressCallback(TProgressCallback const & progressFn);
  void SetPointCheckCallback(TPointCheckCallback const & pointFn);

private:
  TProgressCallback m_progressFn;
  TPointCheckCallback m_pointFn;
};

}  //  nomespace routing
