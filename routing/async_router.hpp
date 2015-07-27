#pragma once

#include "online_absent_fetcher.hpp"
#include "route.hpp"
#include "router.hpp"
#include "timeout_observer.hpp"

#include "std/atomic.hpp"
#include "std/map.hpp"
#include "std/mutex.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"

namespace routing
{

class AsyncRouter
{
public:
  /// Callback takes ownership of passed route.
  using TReadyCallback = function<void(Route &, IRouter::ResultCode)>;

  /// Callback on routing statistics
  using TRoutingStatisticsCallback = function<void(map<string, string> const &)>;

  /// AsyncRouter is a wrapper class to run routing routines in the different thread
  /// @param router pointer to the router implementation. AsyncRouter will take ownership over
  /// router.
  AsyncRouter(unique_ptr<IRouter> && router, unique_ptr<OnlineAbsentCountriesFetcher> && fetcher,
              TRoutingStatisticsCallback const & routingStatisticsFn,
              TPointCheckCallback const & pointCheckCallback);

  virtual ~AsyncRouter();

  /// Main method to calulate new route from startPt to finalPt with start direction
  /// Processed result will be passed to callback. Callback will called at GUI thread.
  ///
  /// @param startPoint point to start routing
  /// @param direction start direction for routers with high cost of the turnarounds
  /// @param finalPoint target point for route
  /// @param readyCallback function to return routing result
  /// @param progressCallback function to update the router progress
  /// @param timeoutSec timeout to cancel routing. 0 is infinity.
  virtual void CalculateRoute(m2::PointD const & startPoint, m2::PointD const & direction,
                              m2::PointD const & finalPoint, TReadyCallback const & readyCallback,
                              TProgressCallback const & progressCallback, uint32_t timeoutSec);

  /// Interrupt routing and clear buffers
  virtual void ClearState();

private:
  /// This function is called in async mode
  void CalculateRouteImpl(TReadyCallback const & readyCallback,
      TProgressCallback const & progressCallback);

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

  mutex m_paramsMutex;
  mutex m_routingMutex;
  atomic_flag m_isReadyThread;

  m2::PointD m_startPoint;
  m2::PointD m_finalPoint;
  m2::PointD m_startDirection;

  TimeoutObserver m_observer;

  unique_ptr<OnlineAbsentCountriesFetcher> const m_absentFetcher;
  unique_ptr<IRouter> const m_router;
  TRoutingStatisticsCallback const m_routingStatisticsFn;
};

}  // namespace routing
