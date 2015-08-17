#include "route.hpp"
#include "turns_generator.hpp"

#include "indexer/mercator.hpp"

#include "platform/location.hpp"

#include "geometry/angles.hpp"
#include "geometry/point2d.hpp"
#include "geometry/simplification.hpp"

#include "base/logging.hpp"

#include "std/numeric.hpp"
#include "std/utility.hpp"
#include "std/algorithm.hpp"


namespace routing
{
namespace
{
double constexpr kLocationTimeThreshold = 60.0 * 1.0;
double constexpr kOnEndToleranceM = 10.0;

}  //  namespace

Route::Route(string const & router, vector<m2::PointD> const & points, string const & name)
  : m_router(router), m_routingSettings(GetCarRoutingSettings()),
    m_poly(points.begin(), points.end()), m_name(name)
{
  Update();
}

void Route::Swap(Route & rhs)
{
  m_router.swap(rhs.m_router);
  swap(m_routingSettings, rhs.m_routingSettings);
  m_poly.Swap(rhs.m_poly);
  m_simplifiedPoly.Swap(m_simplifiedPoly);
  m_name.swap(rhs.m_name);
  swap(m_currentTime, rhs.m_currentTime);
  swap(m_turns, rhs.m_turns);
  swap(m_times, rhs.m_times);
  m_absentCountries.swap(rhs.m_absentCountries);
//  m_simplifiedPoly.Swap(rhs.m_simplifiedPoly);
}

double Route::GetTotalDistanceMeters() const
{
  ASSERT(m_poly.IsValid(), ());
  return m_poly.GetDistanceM(m_poly.Begin(), m_poly.End());
}

double Route::GetCurrentDistanceFromBeginMeters() const
{
  ASSERT(m_poly.IsValid(), ());
  return m_poly.GetDistanceM(m_poly.Begin(), m_poly.GetCurrentIter());
}

void Route::GetTurnsDistances(vector<double> & distances) const
{
  double mercatorDistance = 0;
  distances.clear();
  for (auto currentTurn = m_turns.begin(); currentTurn != m_turns.end(); ++currentTurn)
  {
    uint32_t formerTurnIndex = 0;
    if (currentTurn != m_turns.begin())
      formerTurnIndex = (currentTurn - 1)->m_index;

    //TODO (ldragunov) Extract CalculateMercatorDistance higher to avoid including turns generator.
    auto const polyline = m_poly.GetPolyline();
    double const mercatorDistanceBetweenTurns =
      turns::CalculateMercatorDistanceAlongPath(formerTurnIndex,  currentTurn->m_index, polyline.GetPoints());
    mercatorDistance += mercatorDistanceBetweenTurns;

    if (currentTurn->m_index == 0 || currentTurn->m_index == (polyline.GetSize() - 1))
      continue;
    distances.push_back(mercatorDistance);
   }
}

double Route::GetCurrentDistanceToEndMeters() const
{
  return m_poly.GetDistanceM(m_poly.GetCurrentIter(), m_poly.End());
}

double Route::GetMercatorDistanceFromBegin() const
{
  //TODO Maybe better to return FollowedRoute and user will call GetMercatorDistance etc. by itself
  return m_poly.GetMercatorDistanceFromBegin();
}

uint32_t Route::GetTotalTimeSec() const
{
  return m_times.empty() ? 0 : m_times.back().second;
}

uint32_t Route::GetCurrentTimeToEndSec() const
{
  auto const & poly = m_poly.GetPolyline();
  size_t const polySz = poly.GetSize();
  if (m_times.empty() || poly.GetSize() == 0)
  {
    ASSERT(!m_times.empty(), ());
    ASSERT(polySz != 0, ());
    return 0;
  }

  TTimes::const_iterator it = upper_bound(m_times.begin(), m_times.end(), m_poly.GetCurrentIter().m_ind,
                                         [](size_t v, Route::TTimeItem const & item) { return v < item.first; });

  if (it == m_times.end())
    return 0;

  size_t idx = distance(m_times.begin(), it);
  double time = (*it).second;
  if (idx > 0)
    time -= m_times[idx - 1].second;

  auto distFn = [&](uint32_t start, uint32_t end)
  {
    if (start > polySz || end > polySz)
    {
      ASSERT(false, ());
      return 0.;
    }
    double d = 0.0;
    for (uint32_t i = start + 1; i < end; ++i)
      d += MercatorBounds::DistanceOnEarth(poly.GetPoint(i - 1), poly.GetPoint(i));
    return d;
  };

  ASSERT_LESS_OR_EQUAL(m_times[idx].first, poly.GetSize(), ());
  double const dist = distFn(idx > 0 ? m_times[idx - 1].first : 0, m_times[idx].first + 1);

  if (!my::AlmostEqualULPs(dist, 0.))
  {
    double const distRemain = distFn(m_poly.GetCurrentIter().m_ind, m_times[idx].first + 1) -
                        MercatorBounds::DistanceOnEarth(m_poly.GetCurrentIter().m_pt, poly.GetPoint(m_poly.GetCurrentIter().m_ind));
    return (uint32_t)((GetTotalTimeSec() - (*it).second) + (double)time * (distRemain / dist));
  }
  else
    return (uint32_t)((GetTotalTimeSec() - (*it).second));
}

void Route::GetCurrentTurn(double & distanceToTurnMeters, turns::TurnItem & turn) const
{
  if (m_turns.empty())
  {
    ASSERT(!m_turns.empty(), ());
    distanceToTurnMeters = 0;
    turn = turns::TurnItem();
    return;
  }

  turns::TurnItem t;
  t.m_index = m_poly.GetCurrentIter().m_ind;
  auto it = upper_bound(m_turns.begin(), m_turns.end(), t,
            [](turns::TurnItem const & lhs, turns::TurnItem const & rhs)
            {
              return lhs.m_index < rhs.m_index;
            });

  ASSERT_GREATER_OR_EQUAL((*it).m_index - 1, 0, ());

  size_t const segIdx = (*it).m_index - 1;
  turn = (*it);
  distanceToTurnMeters = m_poly.GetDistanceM(m_poly.GetCurrentIter(),
                                             m_poly.GetIterToIndex(segIdx));
}

void Route::GetCurrentDirectionPoint(m2::PointD & pt) const
{
  if (m_routingSettings.m_keepPedestrianInfo)
    m_simplifiedPoly.GetCurrentDirectionPoint(pt, kOnEndToleranceM);
  else
    m_poly.GetCurrentDirectionPoint(pt, kOnEndToleranceM);
}

bool Route::MoveIterator(location::GpsInfo const & info) const
{
  double predictDistance = -1.0;
  if (m_currentTime > 0.0 && info.HasSpeed())
  {
    /// @todo Need to distinguish GPS and WiFi locations.
    /// They may have different time metrics in case of incorrect system time on a device.
    double const deltaT = info.m_timestamp - m_currentTime;
    if (deltaT > 0.0 && deltaT < kLocationTimeThreshold)
      predictDistance = info.m_speed * deltaT;
  }

  m2::RectD const rect = MercatorBounds::MetresToXY(
        info.m_longitude, info.m_latitude,
        max(m_routingSettings.m_matchingThresholdM, info.m_horizontalAccuracy));
  FollowedPolyline::Iter const res = m_poly.UpdateProjectionByPrediction(rect, predictDistance);
  if (m_simplifiedPoly.IsValid())
    m_simplifiedPoly.UpdateProjectionByPrediction(rect, predictDistance);
  return res.IsValid();
}

double Route::GetCurrentSqDistance(m2::PointD const & pt) const
{
  ASSERT(m_poly.IsValid(), ());
  return pt.SquareLength(m_poly.GetCurrentIter().m_pt);
}

double Route::GetPolySegAngle(size_t ind) const
{
  size_t const polySz = m_poly.GetPolyline().GetSize();

  if (ind + 1 >= polySz)
  {
    ASSERT(false, ());
    return 0;
  }

  m2::PointD const p1 = m_poly.GetPolyline().GetPoint(ind);
  m2::PointD p2;
  size_t i = ind + 1;
  do
  {
    p2 = m_poly.GetPolyline().GetPoint(i);
  }
  while (m2::AlmostEqualULPs(p1, p2) && ++i < polySz);
  return (i == polySz) ? 0 : my::RadToDeg(ang::AngleTo(p1, p2));
}

void Route::MatchLocationToRoute(location::GpsInfo & location, location::RouteMatchingInfo & routeMatchingInfo) const
{
  if (m_poly.IsValid())
  {
    m2::PointD const locationMerc = MercatorBounds::FromLatLon(location.m_latitude, location.m_longitude);
    double const distFromRouteM = MercatorBounds::DistanceOnEarth(m_poly.GetCurrentIter().m_pt, locationMerc);
    if (distFromRouteM < m_routingSettings.m_matchingThresholdM)
    {
      location.m_latitude = MercatorBounds::YToLat(m_poly.GetCurrentIter().m_pt.y);
      location.m_longitude = MercatorBounds::XToLon(m_poly.GetCurrentIter().m_pt.x);
      if (m_routingSettings.m_matchRoute)
        location.m_bearing = location::AngleToBearing(GetPolySegAngle(m_poly.GetCurrentIter().m_ind));

      routeMatchingInfo.Set(m_poly.GetCurrentIter().m_pt, m_poly.GetCurrentIter().m_ind);
    }
  }
}

bool Route::IsCurrentOnEnd() const
{
  return (m_poly.GetDistanceM(m_poly.GetCurrentIter(), m_poly.End()) < kOnEndToleranceM);
}

void Route::Update()
{
  if (!m_poly.IsValid())
    return;
  if (m_routingSettings.m_keepPedestrianInfo)
  {
    vector<m2::PointD> points;
    auto distFn = m2::DistanceToLineSquare<m2::PointD>();
    // TODO (ldargunov) Rewrite dist f to distance in meters and avoid 0.00000 constants.
    SimplifyNearOptimal(20, m_poly.GetPolyline().Begin(), m_poly.GetPolyline().End(), 0.00000001, distFn,
                        MakeBackInsertFunctor(points));
    m_simplifiedPoly = FollowedPolyline(points.begin(), points.end());
  }
  else
  {
    // Free memory if we need no this geometry.
    m_simplifiedPoly = FollowedPolyline();
  }
  m_currentTime = 0.0;
}

string DebugPrint(Route const & r)
{
  return DebugPrint(r.m_poly.GetPolyline());
}

} // namespace routing
