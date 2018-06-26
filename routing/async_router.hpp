#pragma once

#include "routing/checkpoints.hpp"
#include "routing/online_absent_fetcher.hpp"
#include "routing/route.hpp"
#include "routing/router.hpp"
#include "routing/router_delegate.hpp"
#include "routing/routing_callbacks.hpp"

#include "platform/platform.hpp"

#include "base/thread.hpp"

#include <condition_variable>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

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
  void SetRouter(std::unique_ptr<IRouter> && router, std::unique_ptr<IOnlineFetcher> && fetcher);

  /// Main method to calulate new route from startPt to finalPt with start direction
  /// Processed result will be passed to callback. Callback will called at GUI thread.
  ///
  /// @param checkpoints start, finish and intermadiate points
  /// @param direction start direction for routers with high cost of the turnarounds
  /// @param adjustToPrevRoute adjust route to the previous one if possible
  /// @param readyCallback function to return routing result
  /// @param progressCallback function to update the router progress
  /// @param timeoutSec timeout to cancel routing. 0 is infinity.
  // @TODO(bykoianko) Gather |readyCallback|, |needMoreMapsCallback| and |removeRouteCallback|
  // to one delegate. No need to add |progressCallback| to the delegate.
  void CalculateRoute(Checkpoints const & checkpoints, m2::PointD const & direction,
                      bool adjustToPrevRoute, ReadyCallbackOwnership const & readyCallback,
                      NeedMoreMapsCallback const & needMoreMapsCallback,
                      RemoveRouteCallback const & removeRouteCallback,
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
  void LogCode(RouterResultCode code, double const elapsedSec);

  /// Blocks callbacks when routing has been cancelled
  class RouterDelegateProxy
  {
  public:
    RouterDelegateProxy(ReadyCallbackOwnership const & onReady,
                        NeedMoreMapsCallback const & onNeedMoreMaps,
                        RemoveRouteCallback const & onRemoveRoute,
                        PointCheckCallback const & onPointCheck,
                        ProgressCallback const & onProgress,
                        uint32_t timeoutSec);

    void OnReady(std::shared_ptr<Route> route, RouterResultCode resultCode);
    void OnNeedMoreMaps(uint64_t routeId, vector<std::string> const & absentCounties);
    void OnRemoveRoute(RouterResultCode resultCode);
    void Cancel();

    RouterDelegate const & GetDelegate() const { return m_delegate; }

  private:
    void OnProgress(float progress);
    void OnPointCheck(m2::PointD const & pt);

    std::mutex m_guard;
    ReadyCallbackOwnership const m_onReadyOwnership;
    // |m_onNeedMoreMaps| may be called after |m_onReadyOwnership| if
    // - it's possible to build route only if to load some maps
    // - there's a faster route, but it's necessary to load some more maps to build it
    NeedMoreMapsCallback const m_onNeedMoreMaps;
    RemoveRouteCallback const m_onRemoveRoute;
    PointCheckCallback const m_onPointCheck;
    ProgressCallback const m_onProgress;
    RouterDelegate m_delegate;
  };

private:
  std::mutex m_guard;

  /// Thread which executes routing calculation
  threads::SimpleThread m_thread;
  std::condition_variable m_threadCondVar;
  bool m_threadExit;
  bool m_hasRequest;

  /// Current request parameters
  bool m_clearState = false;
  Checkpoints m_checkpoints;
  m2::PointD m_startDirection = m2::PointD::Zero();
  bool m_adjustToPrevRoute = false;
  std::shared_ptr<RouterDelegateProxy> m_delegate;
  std::shared_ptr<IOnlineFetcher> m_absentFetcher;
  std::shared_ptr<IRouter> m_router;

  RoutingStatisticsCallback const m_routingStatisticsCallback;
  PointCheckCallback const m_pointCheckCallback;
  uint64_t m_routeCounter = 0;
};
}  // namespace routing
