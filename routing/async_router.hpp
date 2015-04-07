#pragma once

#include "route.hpp"
#include "router.hpp"

#include "../std/atomic.hpp"
#include "../base/mutex.hpp"

namespace routing
{
class AsyncRouter : public IRouter
{
public:
  /// AsyncRouter is a wrapper class to run routing routines in the different thread
  AsyncRouter();

  typedef atomic<bool> CancelFlagT;

  /// Main method to calulate new route from startPt to finalPt with start direction
  /// Processed result will be passed to callback. Callback will called at GUI thread.
  ///
  /// @param startPoint point to start routing
  /// @param direction start direction for routers with high cost of the turnarounds
  /// @param finalPoint target point for route
  /// @param callback function to return routing result
  void CalculateRoute(m2::PointD const & startPoint, m2::PointD const & direction,
                      m2::PointD const & finalPoint, ReadyCallback const & callback) override;

  /// Calculate route in main thread
  /// @param startPoint point to start routing
  /// @param direction start direction for routers with high cost of the turnarounds
  /// @param finalPoint target point for route
  /// @param route result route
  /// @return ResultCode error code or NoError if route was initialised
  ResultCode CalculateRouteSync(m2::PointD const & startPoint, m2::PointD const & startDirection,
                                m2::PointD const & finalPoint, Route & route)
  {
    return CalculateRouteImpl(startPoint, startDirection, finalPoint, route);
  }

  /// Interrupt routing and clear buffers
  virtual void ClearState();

protected:
  atomic<bool> m_requestCancel;

private:
  /// Override this function with routing implementation.
  /// It will be called in separate thread and only one function will processed in same time.
  /// All locks implemented in AsyncRouter wrapper.
  /// @warning you must process m_requestCancel interruption flag. You must stop processing when it is true.
  ///
  /// @param startPt point to start routing
  /// @param direction start direction for routers with high cost of the turnarounds
  /// @param finalPt target point for route
  /// @param route result route
  /// @return ResultCode error code or NoError if route was initialised
  virtual ResultCode CalculateRouteImpl(m2::PointD const & startPoint, m2::PointD const & startDirection,
                                        m2::PointD const & finalPoint, Route & route) = 0;

  void CalculateRouteAsync(ReadyCallback const & callback);

  threads::Mutex m_paramsMutex;
  threads::Mutex m_routeMutex;
  atomic_flag m_isReadyThread;

  m2::PointD m_startPoint, m_finalPoint, m_startDirection;
};
}
