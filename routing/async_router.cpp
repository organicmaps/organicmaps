#include "routing/async_router.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/timer.hpp"

#include <functional>
#include <mutex>
#include <utility>

namespace routing
{
using std::lock_guard, std::unique_lock;
// ----------------------------------------------------------------------------------------------------------------------------

AsyncRouter::RouterDelegateProxy::RouterDelegateProxy(ReadyCallbackOwnership const & onReady,
                                                      NeedMoreMapsCallback const & onNeedMoreMaps,
                                                      RemoveRouteCallback const & onRemoveRoute,
                                                      PointCheckCallback const & onPointCheck,
                                                      ProgressCallback const & onProgress, uint32_t timeoutSec)
  : m_onReadyOwnership(onReady)
  , m_onNeedMoreMaps(onNeedMoreMaps)
  , m_onRemoveRoute(onRemoveRoute)
  , m_onPointCheck(onPointCheck)
  , m_onProgress(onProgress)
{
  m_delegate.Reset();
  m_delegate.SetPointCheckCallback(std::bind(&RouterDelegateProxy::OnPointCheck, this, std::placeholders::_1));
  m_delegate.SetProgressCallback(std::bind(&RouterDelegateProxy::OnProgress, this, std::placeholders::_1));
  m_delegate.SetTimeout(timeoutSec);
}

void AsyncRouter::RouterDelegateProxy::OnReady(std::shared_ptr<Route> route, RouterResultCode resultCode)
{
  if (!m_onReadyOwnership)
    return;
  {
    lock_guard l(m_guard);
    if (m_delegate.IsCancelled())
      return;
  }
  m_onReadyOwnership(std::move(route), resultCode);
}

void AsyncRouter::RouterDelegateProxy::OnNeedMoreMaps(uint64_t routeId, std::set<std::string> const & absentCounties)
{
  if (!m_onNeedMoreMaps)
    return;
  {
    lock_guard l(m_guard);
    if (m_delegate.IsCancelled())
      return;
  }
  m_onNeedMoreMaps(routeId, absentCounties);
}

void AsyncRouter::RouterDelegateProxy::OnRemoveRoute(RouterResultCode resultCode)
{
  if (!m_onRemoveRoute)
    return;
  {
    lock_guard l(m_guard);
    if (m_delegate.IsCancelled())
      return;
  }
  m_onRemoveRoute(resultCode);
}

void AsyncRouter::RouterDelegateProxy::Cancel()
{
  lock_guard l(m_guard);
  m_delegate.Cancel();
}

bool AsyncRouter::FindClosestProjectionToRoad(m2::PointD const & point, m2::PointD const & direction, double radius,
                                              EdgeProj & proj)
{
  return m_router->FindClosestProjectionToRoad(point, direction, radius, proj);
}

void AsyncRouter::RouterDelegateProxy::OnProgress(float progress)
{
  ProgressCallback onProgress = nullptr;

  {
    lock_guard l(m_guard);
    if (!m_onProgress)
      return;

    if (m_delegate.IsCancelled())
      return;

    onProgress = m_onProgress;
    GetPlatform().RunTask(Platform::Thread::Gui, [onProgress, progress]() { onProgress(progress); });
  }
}

void AsyncRouter::RouterDelegateProxy::OnPointCheck(ms::LatLon const & pt)
{
#ifdef SHOW_ROUTE_DEBUG_MARKS
  PointCheckCallback onPointCheck = nullptr;
  m2::PointD point;
  {
    lock_guard<mutex> l(m_guard);
    CHECK(m_onPointCheck, ());

    if (m_delegate.IsCancelled())
      return;

    onPointCheck = m_onPointCheck;
    point = mercator::FromLatLon(pt);
  }

  GetPlatform().RunTask(Platform::Thread::Gui, [onPointCheck, point]() { onPointCheck(point); });
#else
  UNUSED_VALUE(pt);
#endif
}

// -------------------------------------------------------------------------------------------------

AsyncRouter::AsyncRouter(PointCheckCallback const & pointCheckCallback)
  : m_threadExit(false)
  , m_hasRequest(false)
  , m_clearState(false)
  , m_pointCheckCallback(pointCheckCallback)
{
  m_thread = threads::SimpleThread(&AsyncRouter::ThreadFunc, this);
}

AsyncRouter::~AsyncRouter()
{
  {
    unique_lock ul(m_guard);

    ResetDelegate();

    m_threadExit = true;
    m_threadCondVar.notify_one();
  }

  m_thread.join();
}

void AsyncRouter::SetRouter(std::unique_ptr<IRouter> && router, std::unique_ptr<AbsentRegionsFinder> && finder)
{
  unique_lock ul(m_guard);

  ResetDelegate();

  m_router = std::move(router);
  m_absentRegionsFinder = std::move(finder);
}

void AsyncRouter::CalculateRoute(Checkpoints const & checkpoints, m2::PointD const & direction, bool adjustToPrevRoute,
                                 ReadyCallbackOwnership const & readyCallback,
                                 NeedMoreMapsCallback const & needMoreMapsCallback,
                                 RemoveRouteCallback const & removeRouteCallback,
                                 ProgressCallback const & progressCallback, uint32_t timeoutSec)
{
  unique_lock ul(m_guard);

  m_checkpoints = checkpoints;
  m_startDirection = direction;
  m_adjustToPrevRoute = adjustToPrevRoute;

  ResetDelegate();

  m_delegateProxy = std::make_shared<RouterDelegateProxy>(readyCallback, needMoreMapsCallback, removeRouteCallback,
                                                          m_pointCheckCallback, progressCallback, timeoutSec);

  m_hasRequest = true;
  m_threadCondVar.notify_one();
}

void AsyncRouter::SetGuidesTracks(GuidesTracks && guides)
{
  unique_lock ul(m_guard);
  m_guides = std::move(guides);
}

void AsyncRouter::ClearState()
{
  unique_lock ul(m_guard);

  m_clearState = true;
  m_threadCondVar.notify_one();

  ResetDelegate();
}

// static
void AsyncRouter::LogCode(RouterResultCode code, double const elapsedSec)
{
  switch (code)
  {
  case RouterResultCode::StartPointNotFound: LOG(LWARNING, ("Can't find start or end node")); break;
  case RouterResultCode::EndPointNotFound: LOG(LWARNING, ("Can't find end point node")); break;
  case RouterResultCode::PointsInDifferentMWM: LOG(LWARNING, ("Points are in different MWMs")); break;
  case RouterResultCode::RouteNotFound: LOG(LWARNING, ("Route not found")); break;
  case RouterResultCode::RouteFileNotExist: LOG(LWARNING, ("There is no routing file")); break;
  case RouterResultCode::NeedMoreMaps:
    LOG(LINFO, ("Routing can find a better way with additional maps, elapsed seconds:", elapsedSec));
    break;
  case RouterResultCode::Cancelled: LOG(LINFO, ("Route calculation cancelled, elapsed seconds:", elapsedSec)); break;
  case RouterResultCode::NoError: LOG(LINFO, ("Route found, elapsed seconds:", elapsedSec)); break;
  case RouterResultCode::NoCurrentPosition: LOG(LINFO, ("No current position")); break;
  case RouterResultCode::InconsistentMWMandRoute: LOG(LINFO, ("Inconsistent mwm and route")); break;
  case RouterResultCode::InternalError: LOG(LINFO, ("Internal error")); break;
  case RouterResultCode::FileTooOld: LOG(LINFO, ("File too old")); break;
  case RouterResultCode::IntermediatePointNotFound: LOG(LWARNING, ("Can't find intermediate point node")); break;
  case RouterResultCode::TransitRouteNotFoundNoNetwork:
    LOG(LWARNING, ("No transit route is found because there's no transit network in the mwm of "
                   "the route point"));
    break;
  case RouterResultCode::TransitRouteNotFoundTooLongPedestrian:
    LOG(LWARNING, ("No transit route is found because pedestrian way is too long"));
    break;
  case RouterResultCode::RouteNotFoundRedressRouteError:
    LOG(LWARNING, ("Route not found because of a redress route error"));
    break;
  case RouterResultCode::HasWarnings: LOG(LINFO, ("Route has warnings, elapsed seconds:", elapsedSec)); break;
  }
}

void AsyncRouter::ResetDelegate()
{
  if (m_delegateProxy)
  {
    m_delegateProxy->Cancel();
    m_delegateProxy.reset();
  }
}

void AsyncRouter::ThreadFunc()
{
  while (true)
  {
    {
      unique_lock ul(m_guard);
      m_threadCondVar.wait(ul, [this]() { return m_threadExit || m_hasRequest || m_clearState; });

      if (m_clearState && m_router)
      {
        m_router->ClearState();
        m_clearState = false;
      }

      if (m_threadExit)
        break;

      if (!m_hasRequest)
        continue;
    }

    CalculateRoute();
  }
}

void AsyncRouter::CalculateRoute()
{
  Checkpoints checkpoints;
  std::shared_ptr<RouterDelegateProxy> delegateProxy;
  m2::PointD startDirection;
  bool adjustToPrevRoute = false;
  std::shared_ptr<AbsentRegionsFinder> absentRegionsFinder;
  std::shared_ptr<IRouter> router;
  uint64_t routeId = 0;
  std::string routerName;

  {
    unique_lock ul(m_guard);

    bool hasRequest = m_hasRequest;
    m_hasRequest = false;
    if (!hasRequest)
      return;
    if (!m_router)
      return;
    if (!m_delegateProxy)
      return;

    checkpoints = m_checkpoints;
    startDirection = m_startDirection;
    adjustToPrevRoute = m_adjustToPrevRoute;
    delegateProxy = m_delegateProxy;
    router = m_router;
    absentRegionsFinder = m_absentRegionsFinder;
    routeId = ++m_routeCounter;
    routerName = router->GetName();
    router->SetGuides(std::move(m_guides));
    m_guides.clear();
  }

  auto route = std::make_shared<Route>(router->GetName(), routeId);
  RouterResultCode code;

  base::Timer timer;
  double elapsedSec = 0.0;

  try
  {
    LOG(LINFO, ("Calculating the route of direct length", checkpoints.GetSummaryLengthBetweenPointsMeters(),
                "m. checkpoints:", checkpoints, "startDirection:", startDirection, "router name:", router->GetName()));

    if (absentRegionsFinder)
      absentRegionsFinder->GenerateAbsentRegions(checkpoints, delegateProxy->GetDelegate());

    // Run basic request.
    code = router->CalculateRoute(checkpoints, startDirection, adjustToPrevRoute, delegateProxy->GetDelegate(), *route);
    router->SetGuides({});
    elapsedSec = timer.ElapsedSeconds();  // routing time
    LogCode(code, elapsedSec);
    LOG(LINFO, ("ETA:", route->GetTotalTimeSec(), "sec."));
  }
  catch (RootException const & e)
  {
    code = RouterResultCode::InternalError;
    LOG(LERROR, ("Exception happened while calculating route:", e.Msg()));
    // Note. After call of this method |route| should be used only on ui thread.
    // And |route| should stop using on routing background thread, in this method.
    GetPlatform().RunTask(Platform::Thread::Gui,
                          [delegateProxy, route, code]() { delegateProxy->OnReady(route, code); });
    return;
  }

  // Draw route without waiting network latency.
  if (code == RouterResultCode::NoError)
  {
    // Note. After call of this method |route| should be used only on ui thread.
    // And |route| should stop using on routing background thread, in this method.
    GetPlatform().RunTask(Platform::Thread::Gui,
                          [delegateProxy, route, code]() { delegateProxy->OnReady(route, code); });
  }

  bool const needAbsentRegions = (code != RouterResultCode::Cancelled && route->GetRouterId() != "ruler-router");

  std::set<std::string> absent;
  if (needAbsentRegions)
  {
    if (absentRegionsFinder)
      absentRegionsFinder->GetAbsentRegions(absent);

    absent.insert(route->GetAbsentCountries().cbegin(), route->GetAbsentCountries().cend());
    if (!absent.empty())
    {
      code = RouterResultCode::NeedMoreMaps;
      LOG(LDEBUG, ("Absent regions:", absent));
    }
  }

  elapsedSec = timer.ElapsedSeconds();  // routing time + absents fetch time
  LogCode(code, elapsedSec);

  // Call callback only if we have some new data.
  if (code != RouterResultCode::NoError)
  {
    if (code == RouterResultCode::NeedMoreMaps)
    {
      GetPlatform().RunTask(Platform::Thread::Gui,
                            [delegateProxy, routeId, absent]() { delegateProxy->OnNeedMoreMaps(routeId, absent); });
    }
    else
    {
      GetPlatform().RunTask(Platform::Thread::Gui, [delegateProxy, code]() { delegateProxy->OnRemoveRoute(code); });
    }
  }
}
}  // namespace routing
