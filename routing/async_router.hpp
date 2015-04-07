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
  /// Processed result will be passed to callback
  ///
  /// @param startPt point to start routing
  /// @param direction start direction for routers with high cost of the turnarounds
  /// @param finalPt target point for route
  /// @param callback function to return routing result
  void CalculateRoute(
      m2::PointD const & startPt,
      m2::PointD const & direction,
      m2::PointD const & finalPt,
      ReadyCallback const & callback) override;

  /// Interrupt routing and clear buffers
  virtual void ClearState();

  /// Override this function with routing implementation.
  /// It will be called in separate thread and only one function will processed in same time.
  /// All locks implemented in AsyncRouter wrapper.
  ///
  /// @param startPt point to start routing
  /// @param direction start direction for routers with high cost of the turnarounds
  /// @param finalPt target point for route
  /// @param requestCancel interruption flag. You must stop processing when it is true.
  /// @param callback function to return routing result
  virtual ResultCode CalculateRouteImpl(m2::PointD const & startPt,
                                        m2::PointD const & startDr,
                                        m2::PointD const & finalPt,
                                        CancelFlagT const & requestCancel,
                                        Route & route) = 0;

private:
  void CalculateRouteAsync(ReadyCallback const & callback);

  threads::Mutex m_paramsMutex;
  threads::Mutex m_routeMutex;
  atomic_flag m_isReadyThread;
  atomic<bool> m_requestCancel;

  m2::PointD m_startPt, m_finalPt, m_startDr;
};

}
