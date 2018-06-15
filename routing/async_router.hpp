#pragma once

#include "routing/checkpoints.hpp"
#include "routing/online_absent_fetcher.hpp"
#include "routing/route.hpp"
#include "routing/router.hpp"
#include "routing/router_delegate.hpp"
#include "routing/routing_callbacks.hpp"

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
  /// AsyncRouter is a wrapper class to run routing routines in the different thread
  AsyncRouter(RoutingStatisticsCallback const & routingStatisticsCallback,
              PointCheckCallback const & pointCheckCallback);
  ~AsyncRouter();

  /// Sets a synchronous router, current route calculation will be cancelled
  /// @param router pointer to a router implementation
  /// @param fetcher pointer to a online fetcher
  void SetRouter(unique_ptr<IRouter> && router, unique_ptr<IOnlineFetcher> && fetcher);

  /// Main method to calulate new route from startPt to finalPt with start direction
  /// Processed result will be passed to callback. Callback will called at GUI thread.
  ///
  /// @param checkpoints start, finish and intermadiate points
  /// @param direction start direction for routers with high cost of the turnarounds
  /// @param adjustToPrevRoute adjust route to the previous one if possible
  /// @param readyCallback function to return routing result
  /// @param progressCallback function to update the router progress
  /// @param timeoutSec timeout to cancel routing. 0 is infinity.
  void CalculateRoute(Checkpoints const & checkpoints, m2::PointD const & direction,
                      bool adjustToPrevRoute, ReadyCallbackOwnership const & readyCallback,
                      ProgressCallback const & progressCallback,
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
                      RouterResultCode resultCode,
                      Route const & route,
                      double elapsedSec);
  void SendStatistics(m2::PointD const & startPoint, m2::PointD const & startDirection,
                      m2::PointD const & finalPoint,
                      string const & exceptionMessage);

  void LogCode(RouterResultCode code, double const elapsedSec);

  /// Blocks callbacks when routing has been cancelled
  class RouterDelegateProxy
  {
  public:
    RouterDelegateProxy(ReadyCallbackOwnership const & onReady,
                        PointCheckCallback const & onPointCheck,
                        ProgressCallback const & onProgress,
                        uint32_t timeoutSec);

    void OnReady(Route & route, RouterResultCode resultCode);
    void Cancel();

    RouterDelegate const & GetDelegate() const { return m_delegate; }

  private:
    void OnProgress(float progress);
    void OnPointCheck(m2::PointD const & pt);

    mutex m_guard;
    ReadyCallbackOwnership const m_onReady;
    PointCheckCallback const m_onPointCheck;
    ProgressCallback const m_onProgress;
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
  bool m_clearState = false;
  Checkpoints m_checkpoints;
  m2::PointD m_startDirection = m2::PointD::Zero();
  bool m_adjustToPrevRoute = false;
  shared_ptr<RouterDelegateProxy> m_delegate;
  shared_ptr<IOnlineFetcher> m_absentFetcher;
  shared_ptr<IRouter> m_router;

  RoutingStatisticsCallback const m_routingStatisticsCallback;
  PointCheckCallback const m_pointCheckCallback;
};

}  // namespace routing
