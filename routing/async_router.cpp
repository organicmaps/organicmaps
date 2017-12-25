#include "routing/async_router.hpp"

#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include "std/functional.hpp"

using namespace std;
using namespace std::placeholders;

namespace routing
{

namespace
{

string ToString(IRouter::ResultCode code)
{
  switch (code)
  {
  case IRouter::NoError: return "NoError";
  case IRouter::Cancelled: return "Cancelled";
  case IRouter::NoCurrentPosition: return "NoCurrentPosition";
  case IRouter::InconsistentMWMandRoute: return "InconsistentMWMandRoute";
  case IRouter::RouteFileNotExist: return "RouteFileNotExist";
  case IRouter::StartPointNotFound: return "StartPointNotFound";
  case IRouter::EndPointNotFound: return "EndPointNotFound";
  case IRouter::PointsInDifferentMWM: return "PointsInDifferentMWM";
  case IRouter::RouteNotFound: return "RouteNotFound";
  case IRouter::InternalError: return "InternalError";
  case IRouter::NeedMoreMaps: return "NeedMoreMaps";
  case IRouter::FileTooOld: return "FileTooOld";
  case IRouter::IntermediatePointNotFound: return "IntermediatePointNotFound";
  }

  string const result = "Unknown IRouter::ResultCode:" + to_string(static_cast<int>(code));
  ASSERT(false, (result));
  return result;
}

map<string, string> PrepareStatisticsData(string const & routerName,
                                          m2::PointD const & startPoint, m2::PointD const & startDirection,
                                          m2::PointD const & finalPoint)
{
  // Coordinates precision in 5 digits after comma corresponds to metres (0,00001degree ~ 1meter),
  // therefore we round coordinates up to 5 digits after comma.
  int constexpr precision = 5;

  return {{"name", routerName},
          {"startLon", strings::to_string_dac(MercatorBounds::XToLon(startPoint.x), precision)},
          {"startLat", strings::to_string_dac(MercatorBounds::YToLat(startPoint.y), precision)},
          {"startDirectionX", strings::to_string_dac(startDirection.x, precision)},
          {"startDirectionY", strings::to_string_dac(startDirection.y, precision)},
          {"finalLon", strings::to_string_dac(MercatorBounds::XToLon(finalPoint.x), precision)},
          {"finalLat", strings::to_string_dac(MercatorBounds::YToLat(finalPoint.y), precision)}};
}

}  // namespace

// ----------------------------------------------------------------------------------------------------------------------------

AsyncRouter::RouterDelegateProxy::RouterDelegateProxy(TReadyCallback const & onReady,
                                                      RouterDelegate::TPointCheckCallback const & onPointCheck,
                                                      RouterDelegate::TProgressCallback const & onProgress,
                                                      uint32_t timeoutSec)
  : m_onReady(onReady), m_onPointCheck(onPointCheck), m_onProgress(onProgress)
{
  m_delegate.Reset();
  m_delegate.SetPointCheckCallback(bind(&RouterDelegateProxy::OnPointCheck, this, _1));
  m_delegate.SetProgressCallback(bind(&RouterDelegateProxy::OnProgress, this, _1));
  m_delegate.SetTimeout(timeoutSec);
}

void AsyncRouter::RouterDelegateProxy::OnReady(Route & route, IRouter::ResultCode resultCode)
{
  if (!m_onReady)
    return;
  {
    lock_guard<mutex> l(m_guard);
    if (m_delegate.IsCancelled())
      return;
  }
  m_onReady(route, resultCode);
}

void AsyncRouter::RouterDelegateProxy::Cancel()
{
  lock_guard<mutex> l(m_guard);
  m_delegate.Cancel();
}

void AsyncRouter::RouterDelegateProxy::OnProgress(float progress)
{
  if (!m_onProgress)
    return;
  {
    lock_guard<mutex> l(m_guard);
    if (m_delegate.IsCancelled())
      return;
  }
  m_onProgress(progress);
}

void AsyncRouter::RouterDelegateProxy::OnPointCheck(m2::PointD const & pt)
{
  if (!m_onPointCheck)
    return;
  {
    lock_guard<mutex> l(m_guard);
    if (m_delegate.IsCancelled())
      return;
  }
  m_onPointCheck(pt);
}

// ----------------------------------------------------------------------------------------------------------------------------

AsyncRouter::AsyncRouter(TRoutingStatisticsCallback const & routingStatisticsCallback,
                         RouterDelegate::TPointCheckCallback const & pointCheckCallback)
    : m_threadExit(false), m_hasRequest(false), m_clearState(false),
      m_routingStatisticsCallback(routingStatisticsCallback),
      m_pointCheckCallback(pointCheckCallback)
{
  m_thread = threads::SimpleThread(&AsyncRouter::ThreadFunc, this);
}

AsyncRouter::~AsyncRouter()
{
  {
    unique_lock<mutex> ul(m_guard);

    ResetDelegate();

    m_threadExit = true;
    m_threadCondVar.notify_one();
  }

  m_thread.join();
}

void AsyncRouter::SetRouter(unique_ptr<IRouter> && router, unique_ptr<IOnlineFetcher> && fetcher)
{
  unique_lock<mutex> ul(m_guard);

  ResetDelegate();

  m_router = move(router);
  m_absentFetcher = move(fetcher);
}

void AsyncRouter::CalculateRoute(Checkpoints const & checkpoints, m2::PointD const & direction,
                                 bool adjustToPrevRoute, TReadyCallback const & readyCallback,
                                 RouterDelegate::TProgressCallback const & progressCallback,
                                 uint32_t timeoutSec)
{
  unique_lock<mutex> ul(m_guard);

  m_checkpoints = checkpoints;
  m_startDirection = direction;
  m_adjustToPrevRoute = adjustToPrevRoute;

  ResetDelegate();

  m_delegate = make_shared<RouterDelegateProxy>(readyCallback, m_pointCheckCallback, progressCallback, timeoutSec);

  m_hasRequest = true;
  m_threadCondVar.notify_one();
}

void AsyncRouter::ClearState()
{
  unique_lock<mutex> ul(m_guard);

  m_clearState = true;
  m_threadCondVar.notify_one();

  ResetDelegate();
}

void AsyncRouter::LogCode(IRouter::ResultCode code, double const elapsedSec)
{
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
      LOG(LWARNING, ("There is no routing file"));
      break;
    case IRouter::NeedMoreMaps:
      LOG(LINFO,
          ("Routing can find a better way with additional maps, elapsed seconds:", elapsedSec));
      break;
    case IRouter::Cancelled:
      LOG(LINFO, ("Route calculation cancelled, elapsed seconds:", elapsedSec));
      break;
    case IRouter::NoError:
      LOG(LINFO, ("Route found, elapsed seconds:", elapsedSec));
      break;
    case IRouter::NoCurrentPosition:
      LOG(LINFO, ("No current position"));
      break;
    case IRouter::InconsistentMWMandRoute:
      LOG(LINFO, ("Inconsistent mwm and route"));
      break;
    case IRouter::InternalError:
      LOG(LINFO, ("Internal error"));
      break;
    case IRouter::FileTooOld:
      LOG(LINFO, ("File too old"));
      break;
    case IRouter::IntermediatePointNotFound:
      LOG(LWARNING, ("Can't find intermediate point node"));
      break;
  }
}

void AsyncRouter::ResetDelegate()
{
  if (m_delegate)
  {
    m_delegate->Cancel();
    m_delegate.reset();
  }
}

void AsyncRouter::ThreadFunc()
{
  while (true)
  {
    {
      unique_lock<mutex> ul(m_guard);
      m_threadCondVar.wait(ul, [this](){ return m_threadExit || m_hasRequest || m_clearState; });

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
  shared_ptr<RouterDelegateProxy> delegate;
  Checkpoints checkpoints;
  m2::PointD startDirection;
  bool adjustToPrevRoute = false;
  shared_ptr<IOnlineFetcher> absentFetcher;
  shared_ptr<IRouter> router;

  {
    unique_lock<mutex> ul(m_guard);

    bool hasRequest = m_hasRequest;
    m_hasRequest = false;
    if (!hasRequest)
      return;
    if (!m_router)
      return;
    if (!m_delegate)
      return;

    checkpoints = m_checkpoints;
    startDirection = m_startDirection;
    adjustToPrevRoute = m_adjustToPrevRoute;
    delegate = m_delegate;
    router = m_router;
    absentFetcher = m_absentFetcher;
  }

  Route route(router->GetName());
  IRouter::ResultCode code;

  my::Timer timer;
  double elapsedSec = 0.0;

  try
  {
    LOG(LINFO, ("Calculating the route. checkpoints:", checkpoints, "startDirection:",
                startDirection, "router name:", router->GetName()));

    if (absentFetcher)
      absentFetcher->GenerateRequest(checkpoints);

    // Run basic request.
    code = router->CalculateRoute(checkpoints, startDirection, adjustToPrevRoute,
                                  delegate->GetDelegate(), route);

    elapsedSec = timer.ElapsedSeconds(); // routing time
    LogCode(code, elapsedSec);
  }
  catch (RootException const & e)
  {
    code = IRouter::InternalError;
    LOG(LERROR, ("Exception happened while calculating route:", e.Msg()));
    SendStatistics(checkpoints.GetStart(), startDirection, checkpoints.GetFinish(), e.Msg());
    delegate->OnReady(route, code);
    return;
  }

  SendStatistics(checkpoints.GetStart(), startDirection, checkpoints.GetFinish(), code, route,
                 elapsedSec);

  // Draw route without waiting network latency.
  if (code == IRouter::NoError)
    delegate->OnReady(route, code);

  bool const needFetchAbsent = (code != IRouter::Cancelled);

  // Check online response if we have.
  vector<string> absent;
  if (absentFetcher && needFetchAbsent)
  {
    absentFetcher->GetAbsentCountries(absent);
    for (string const & country : absent)
      route.AddAbsentCountry(country);
  }

  if (!absent.empty() && code == IRouter::NoError)
    code = IRouter::NeedMoreMaps;

  elapsedSec = timer.ElapsedSeconds(); // routing time + absents fetch time
  LogCode(code, elapsedSec);

  // Call callback only if we have some new data.
  if (code != IRouter::NoError)
    delegate->OnReady(route, code);
}

void AsyncRouter::SendStatistics(m2::PointD const & startPoint, m2::PointD const & startDirection,
                                 m2::PointD const & finalPoint,
                                 IRouter::ResultCode resultCode,
                                 Route const & route,
                                 double elapsedSec)
{
  if (nullptr == m_routingStatisticsCallback)
    return;

  map<string, string> statistics = PrepareStatisticsData(m_router->GetName(), startPoint, startDirection, finalPoint);
  statistics.emplace("result", ToString(resultCode));
  statistics.emplace("elapsed", strings::to_string(elapsedSec));

  if (IRouter::NoError == resultCode)
    statistics.emplace("distance", strings::to_string(route.GetTotalDistanceMeters()));

  m_routingStatisticsCallback(statistics);
}

void AsyncRouter::SendStatistics(m2::PointD const & startPoint, m2::PointD const & startDirection,
                                 m2::PointD const & finalPoint,
                                 string const & exceptionMessage)
{
  if (nullptr == m_routingStatisticsCallback)
    return;

  map<string, string> statistics = PrepareStatisticsData(m_router->GetName(), startPoint, startDirection, finalPoint);
  statistics.emplace("exception", exceptionMessage);

  m_routingStatisticsCallback(statistics);
}

}  // namespace routing
