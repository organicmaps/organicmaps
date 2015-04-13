#include "routing/async_router.hpp"

#include "platform/platform.hpp"
#include "base/macros.hpp"
#include "base/logging.hpp"

namespace routing
{
AsyncRouter::AsyncRouter(unique_ptr<IRouter> && router) : m_router(move(router))
{
  m_isReadyThread.clear();
}

AsyncRouter::~AsyncRouter() { ClearState(); }

void AsyncRouter::CalculateRoute(m2::PointD const & startPoint, m2::PointD const & direction,
                                 m2::PointD const & finalPoint, ReadyCallback const & callback)
{
  ASSERT(m_router, ());
  {
    lock_guard<mutex> guard(m_paramsMutex);
    UNUSED_VALUE(guard);

    m_startPoint = startPoint;
    m_startDirection = direction;
    m_finalPoint = finalPoint;

    m_router->Cancel();
  }

  GetPlatform().RunAsync(bind(&AsyncRouter::CalculateRouteImpl, this, callback));
}

void AsyncRouter::ClearState()
{
  ASSERT(m_router, ());
  m_router->Cancel();

  lock_guard<mutex> guard(m_routeMutex);
  m_router->ClearState();
}

void AsyncRouter::CalculateRouteImpl(ReadyCallback const & callback)
{
  if (m_isReadyThread.test_and_set())
    return;

  Route route(m_router->GetName());
  IRouter::ResultCode code;

  lock_guard<mutex> guard(m_routeMutex);

  m_isReadyThread.clear();

  m2::PointD startPoint, finalPoint, startDirection;
  {
    lock_guard<mutex> params(m_paramsMutex);

    startPoint = m_startPoint;
    finalPoint = m_finalPoint;
    startDirection = m_startDirection;

    m_router->Reset();
  }

  try
  {
    code = m_router->CalculateRoute(startPoint, startDirection, finalPoint, route);
    switch (code)
    {
      case IRouter::StartPointNotFound:
        LOG(LWARNING, ("Can't find start or end node"));
        break;
      case IRouter::EndPointNotFound:
        LOG(LWARNING, ("Can't find end point node"));
        break;
      case IRouter::PointsInDifferentMWM:
        LOG(LWARNING, ("Points are in different MWMs"));
        break;
      case IRouter::RouteNotFound:
        LOG(LWARNING, ("Route not found"));
        break;
      case IRouter::RouteFileNotExist:
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
    code = IRouter::InternalError;
  }

  GetPlatform().RunOnGuiThread(bind(callback, route, code));
}
}
