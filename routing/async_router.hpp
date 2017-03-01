#pragma once

#include "routing/online_absent_fetcher.hpp"
#include "routing/route.hpp"
#include "routing/router.hpp"
#include "routing/router_delegate.hpp"

#include "base/thread.hpp"

#include "std/condition_variable.hpp"
#include "std/map.hpp"
#include "std/mutex.hpp"
#include "std/shared_ptr.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"

namespace routing
{

/// Dispatches a route calculation on a worker thread
class AsyncRouter final
{
public:
  /// Callback takes ownership of passed route.
  using TReadyCallback = function<void(Route &, IRouter::ResultCode)>;

  /// Callback on routing statistics
  using TRoutingStatisticsCallback = function<void(map<string, string> const &)>;

  /// AsyncRouter is a wrapper class to run routing routines in the different thread
  AsyncRouter(TRoutingStatisticsCallback const & routingStatisticsCallback,
              RouterDelegate::TPointCheckCallback const & pointCheckCallback);
  ~AsyncRouter();

  /// Sets a synchronous router, current route calculation will be cancelled
  /// @param router pointer to a router implementation
  /// @param fetcher pointer to a online fetcher
  void SetRouter(unique_ptr<IRouter> && router, unique_ptr<IOnlineFetcher> && fetcher);

  /// Main method to calulate new route from startPt to finalPt with start direction
  /// Processed result will be passed to callback. Callback will called at GUI thread.
  ///
  /// @param startPoint point to start routing
  /// @param direction start direction for routers with high cost of the turnarounds
  /// @param finalPoint target point for route
  /// @param readyCallback function to return routing result
  /// @param progressCallback function to update the router progress
  /// @param timeoutSec timeout to cancel routing. 0 is infinity.
  void CalculateRoute(m2::PointD const & startPoint, m2::PointD const & direction,
                      m2::PointD const & finalPoint, TReadyCallback const & readyCallback,
                      RouterDelegate::TProgressCallback const & progressCallback,
                      uint32_t timeoutSec);

  /// Interrupt routing and clear buffers
  void ClearState();

private:
  /// Worker thread function
  void ThreadFunc();

  /// This function is called in worker thread
  void CalculateRoute();

  void ResetDelegate();

  /// These functions are called to send statistics about the routing
  void SendStatistics(m2::PointD const & startPoint, m2::PointD const & startDirection,
                      m2::PointD const & finalPoint,
                      IRouter::ResultCode resultCode,
                      Route const & route,
                      double elapsedSec);
  void SendStatistics(m2::PointD const & startPoint, m2::PointD const & startDirection,
                      m2::PointD const & finalPoint,
                      string const & exceptionMessage);

  void LogCode(IRouter::ResultCode code, double const elapsedSec);

  /// Blocks callbacks when routing has been cancelled
  class RouterDelegateProxy
  {
  public:
    RouterDelegateProxy(TReadyCallback const & onReady,
                        RouterDelegate::TPointCheckCallback const & onPointCheck,
                        RouterDelegate::TProgressCallback const & onProgress,
                        uint32_t timeoutSec);

    void OnReady(Route & route, IRouter::ResultCode resultCode);
    void Cancel();

    RouterDelegate const & GetDelegate() const { return m_delegate; }

  private:
    void OnProgress(float progress);
    void OnPointCheck(m2::PointD const & pt);

    mutex m_guard;
    TReadyCallback const m_onReady;
    RouterDelegate::TPointCheckCallback const m_onPointCheck;
    RouterDelegate::TProgressCallback const m_onProgress;
    RouterDelegate m_delegate;
  };

private:
  mutex m_guard;

  /// Thread which executes routing calculation
  threads::SimpleThread m_thread;
  condition_variable m_threadCondVar;
  bool m_threadExit;
  bool m_hasRequest;

  /// Current request parameters
  bool m_clearState;
  m2::PointD m_startPoint;
  m2::PointD m_finalPoint;
  m2::PointD m_startDirection;
  shared_ptr<RouterDelegateProxy> m_delegate;
  shared_ptr<IOnlineFetcher> m_absentFetcher;
  shared_ptr<IRouter> m_router;

  TRoutingStatisticsCallback const m_routingStatisticsCallback;
  RouterDelegate::TPointCheckCallback const m_pointCheckCallback;
};

}  // namespace routing
