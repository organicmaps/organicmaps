#pragma once

#include "router.hpp"

#include "base/timer.hpp"

namespace routing
{
using TProgressCallback = function<void(float)>;
using TPointCheckCallback = function<void(m2::PointD const &)>;

class TimeoutObserver : public IRouterObserver
{
public:
  TimeoutObserver();

  void SetTimeout(uint32_t timeoutSec);
  bool IsCancelled() const override;

  void OnProgress(float progress) const override { m_progressFn(progress); }
  void OnPointCheck(m2::PointD const & point) const override { m_pointFn(point); }

  void SetProgressCallback(TProgressCallback const & progressFn);
  void SetPointCheckCallback(TPointCheckCallback const & pointFn);

private:
  my::Timer m_timer;
  uint32_t m_timeoutSec;

  TProgressCallback m_progressFn;
  TPointCheckCallback m_pointFn;
};

}  //  nomespace routing
