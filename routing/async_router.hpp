#pragma once

#include "routing/route.hpp"
#include "routing/router.hpp"

#include "std/atomic.hpp"
#include "std/mutex.hpp"
#include "std/unique_ptr.hpp"

namespace routing
{
/// Callback takes ownership of passed route.
typedef function<void(Route &, IRouter::ResultCode)> ReadyCallback;

class AsyncRouter
{
public:
  /// AsyncRouter is a wrapper class to run routing routines in the different thread
  /// @param router pointer to the router implementation. AsyncRouter will take ownership over
  /// router.
  AsyncRouter(unique_ptr<IRouter> && router);

  virtual ~AsyncRouter();

  /// Main method to calulate new route from startPt to finalPt with start direction
  /// Processed result will be passed to callback. Callback will called at GUI thread.
  ///
  /// @param startPoint point to start routing
  /// @param direction start direction for routers with high cost of the turnarounds
  /// @param finalPoint target point for route
  /// @param callback function to return routing result
  virtual void CalculateRoute(m2::PointD const & startPoint, m2::PointD const & direction,
                              m2::PointD const & finalPoint, ReadyCallback const & callback);

  /// Interrupt routing and clear buffers
  virtual void ClearState();

private:
  /// This function is called in async mode
  void CalculateRouteImpl(ReadyCallback const & callback);

  mutex m_paramsMutex;
  mutex m_routeMutex;
  atomic_flag m_isReadyThread;

  m2::PointD m_startPoint;
  m2::PointD m_finalPoint;
  m2::PointD m_startDirection;

  unique_ptr<IRouter> m_router;
};
}
