#include "async_router.hpp"

#include "../base/macros.hpp"
#include "../platform/platform.hpp"
#include "../base/logging.hpp"

namespace routing
{
AsyncRouter::AsyncRouter()
{
  m_requestCancel = false;
  m_isReadyThread.clear();
}

void AsyncRouter::CalculateRoute(m2::PointD const & startPoint, m2::PointD const & direction,
                                 m2::PointD const & finalPoint, ReadyCallback const & callback)
{
  {
    threads::MutexGuard guard(m_paramsMutex);
    UNUSED_VALUE(guard);

    m_startPoint = startPoint;
    m_startDirection = direction;
    m_finalPoint = finalPoint;

    m_requestCancel = true;
  }

  GetPlatform().RunAsync(bind(&AsyncRouter::CalculateRouteAsync, this, callback));
}

void AsyncRouter::ClearState()
{
  m_requestCancel = true;

  threads::MutexGuard guard(m_routeMutex);
  UNUSED_VALUE(guard);
}

void AsyncRouter::CalculateRouteAsync(ReadyCallback const & callback)
{
  if (m_isReadyThread.test_and_set())
    return;

  Route route(GetName());
  ResultCode code;

  threads::MutexGuard guard(m_routeMutex);
  UNUSED_VALUE(guard);

  m_isReadyThread.clear();

  m2::PointD startPoint, finalPoint, startDirection;
  {
    threads::MutexGuard params(m_paramsMutex);
    UNUSED_VALUE(params);

    startPoint = m_startPoint;
    finalPoint = m_finalPoint;
    startDirection = m_startDirection;

    m_requestCancel = false;
  }

  try
  {
    code = CalculateRouteImpl(startPoint, startDirection, finalPoint, route);
    switch (code)
    {
      case StartPointNotFound:
        LOG(LWARNING, ("Can't find start or end node"));
        break;
      case EndPointNotFound:
        LOG(LWARNING, ("Can't find end point node"));
        break;
      case PointsInDifferentMWM:
        LOG(LWARNING, ("Points are in different MWMs"));
        break;
      case RouteNotFound:
        LOG(LWARNING, ("Route not found"));
        break;
      case RouteFileNotExist:
        LOG(LWARNING, ("There are no routing file"));
        break;

      default:
        break;
    }
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR,
        ("Routing index is absent or incorrect. Error while loading routing index:", e.Msg()));
    code = InternalError;
  }

  GetPlatform().RunOnGuiThread(bind(callback, route, code));
}
}
