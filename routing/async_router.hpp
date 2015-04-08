#pragma once

#include "route.hpp"
#include "router.hpp"

#include "../base/mutex.hpp"

#include "../std/atomic.hpp"
#include "../std/unique_ptr.hpp"

namespace routing
{

/// Callback takes ownership of passed route.
typedef function<void (Route &, IRouter::ResultCode)> ReadyCallback;

class AsyncRouter
{
public:
  /// AsyncRouter is a wrapper class to run routing routines in the different thread
  /// @param router pointer to the router realisation. AsyncRouter will destroy's router
  AsyncRouter(IRouter * router);

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

  virtual ~AsyncRouter();

private:
  void CalculateRouteAsync(ReadyCallback const & callback);

  threads::Mutex m_paramsMutex;
  threads::Mutex m_routeMutex;
  atomic_flag m_isReadyThread;

  m2::PointD m_startPoint, m_finalPoint, m_startDirection;

  unique_ptr<IRouter> m_router;
};
}
