#include "map/nogps/path_matcher.hpp"

#include "map/nogps/geo.hpp"
#include "map/nogps/road_walker.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <cmath>

namespace nogps
{
namespace
{
double constexpr kHistoryStepM = 2;

double DiffToRoad(double roadBearingDeg, double headingDeg)
{
  return std::fabs(AngleDiff(Orient(roadBearingDeg, headingDeg), headingDeg));
}

// The places of the road every search step around its point, looking the car's way.
std::vector<RoadPoint> PlacesAlong(Roads & roads, RoadPoint const & road, double headingDeg)
{
  std::vector<RoadPoint> places = {{road.m_point, Orient(road.m_bearingDeg, headingDeg)}};
  for (bool const forward : {true, false})
  {
    RoadPoint place = places.front();
    for (double walked = PathMatcher::kSearchStepM; walked <= PathMatcher::kSearchAlongM;
         walked += PathMatcher::kSearchStepM)
    {
      double const moveBearing = forward ? place.m_bearingDeg : place.m_bearingDeg + 180;
      auto const next = roads.Snap(Move(place.m_point, moveBearing, PathMatcher::kSearchStepM), place.m_bearingDeg,
                                   RoadWalker::kSnapRadiusM);
      if (!next || DiffToRoad(next->m_bearingDeg, place.m_bearingDeg) > RoadWalker::kMaxBendDeg)
        break;
      place = {next->m_point, Orient(next->m_bearingDeg, place.m_bearingDeg)};
      places.push_back(place);
    }
  }
  return places;
}
}  // namespace

void PathMatcher::OnMotion(double yawDeltaDeg, double distanceM, double headingDeg)
{
  m_yawDeg += yawDeltaDeg;
  m_odometerM += distanceM;
  if (!m_history.empty() && m_odometerM - m_history.back().m_odometerM < kHistoryStepM)
    return;
  m_history.push_back({m_odometerM, m_yawDeg, headingDeg});
  // One point at the start of the window or before it is kept.
  while (m_history.size() > 2 && m_odometerM - m_history[1].m_odometerM >= kWindowM)
    m_history.pop_front();
}

std::vector<PathMatcher::Point> PathMatcher::GetWay() const
{
  if (m_history.empty() || m_odometerM - m_history.front().m_odometerM < kMinWindowM)
    return {};
  // The way is turned as the car looked at its start, on the road it came along: after a loop beside a ramp the
  // calculated heading is wrong by degrees, and the far end of the way would be meters aside.
  double const turnDeg = m_history.front().m_headingDeg - m_history.front().m_yawDeg;
  std::vector<Point> way = {{0, 0, Normalize(m_yawDeg + turnDeg)}};
  double east = 0;
  double north = 0;
  double walkedM = 0;
  double odometerM = m_odometerM;
  double yawDeg = m_yawDeg;
  for (auto it = m_history.rbegin(); it != m_history.rend() && walkedM < kWindowM; ++it)
  {
    double const length = odometerM - it->m_odometerM;
    if (length <= 0)
      continue;
    double const pieceDeg = (yawDeg + it->m_yawDeg) / 2 + turnDeg;
    double const dEast = std::sin(math::DegToRad(pieceDeg));
    double const dNorth = std::cos(math::DegToRad(pieceDeg));
    for (double checkM = kCheckStepM * way.size(); checkM <= walkedM + length && checkM <= kWindowM;
         checkM = kCheckStepM * way.size())
    {
      double const part = checkM - walkedM;
      way.push_back({east - part * dEast, north - part * dNorth,
                     Normalize(yawDeg + (it->m_yawDeg - yawDeg) * part / length + turnDeg)});
    }
    east -= length * dEast;
    north -= length * dNorth;
    walkedM += length;
    odometerM = it->m_odometerM;
    yawDeg = it->m_yawDeg;
  }
  return way;
}

int PathMatcher::CountMisses(Roads & roads, ms::LatLon const & end, std::vector<Point> const & way, int maxMisses)
{
  int misses = 0;
  for (size_t i = 1; i < way.size() && misses <= maxMisses; ++i)
  {
    auto const & p = way[i];
    auto const point = Move(end, math::RadToDeg(std::atan2(p.m_eastM, p.m_northM)), std::hypot(p.m_eastM, p.m_northM));
    auto const road = roads.Snap(point, p.m_headingDeg, kMaxRoadM);
    if (!road || Distance(point, road->m_point) > kMaxRoadM ||
        DiffToRoad(road->m_bearingDeg, p.m_headingDeg) > kMaxRoadDiffDeg)
      ++misses;
  }
  return misses;
}

void PathMatcher::SetHeading(double headingDeg)
{
  for (auto & sample : m_history)
    sample.m_headingDeg = Normalize(headingDeg - (m_yawDeg - sample.m_yawDeg));
}

std::optional<int> PathMatcher::CountMissesAt(Roads & roads, ms::LatLon const & end) const
{
  auto const way = GetWay();
  if (way.empty())
    return {};
  return CountMisses(roads, end, way, static_cast<int>(way.size()));
}

std::optional<RoadPoint> PathMatcher::Match(Roads & roads, ms::LatLon const & position, std::string * why) const
{
  auto const no = [why](std::string const & text)
  {
    if (why)
      *why = text;
    return std::optional<RoadPoint>();
  };
  auto const way = GetWay();
  if (way.empty())
    return no("too short a way driven");
  // All the points are counted for the log.
  int const missed = CountMisses(roads, position, way, static_cast<int>(way.size()));
  std::string const off =
      std::to_string(missed) + " of " + std::to_string(way.size() - 1) + " points off the roads where the car is";
  if (missed < kMinMissesToMove)
    return no("the way driven lies on the roads, " + off);

  struct Fit
  {
    RoadPoint m_place;
    int m_misses;
  };
  std::vector<Fit> fits;
  size_t tried = 0;
  double const headingDeg = way.front().m_headingDeg;
  for (auto const & road : roads.SnapAll(position, headingDeg, kSearchM))
  {
    if (DiffToRoad(road.m_bearingDeg, headingDeg) > kMaxRoadDiffDeg)
      continue;
    for (auto const & place : PlacesAlong(roads, road, headingDeg))
    {
      ++tried;
      if (int const misses = CountMisses(roads, place.m_point, way, kMaxMisses); misses <= kMaxMisses)
        fits.push_back({place, misses});
    }
  }
  if (fits.empty())
    return no("the way driven lies on no roads around, places tried " + std::to_string(tried) + ", " + off);

  auto const best = std::min_element(fits.begin(), fits.end(), [&position](Fit const & a, Fit const & b)
  {
    if (a.m_misses != b.m_misses)
      return a.m_misses < b.m_misses;
    return Distance(a.m_place.m_point, position) < Distance(b.m_place.m_point, position);
  });
  for (auto const & fit : fits)
  {
    if (Distance(fit.m_place.m_point, best->m_place.m_point) > kSamePlaceM)
    {
      return no("the way driven lies on the roads in places " +
                std::to_string(std::lround(Distance(fit.m_place.m_point, best->m_place.m_point))) + " m apart, " + off);
    }
  }
  if (double const distanceM = Distance(best->m_place.m_point, position); distanceM < kMinMoveM)
  {
    return no("the way driven lies on the roads " + std::to_string(std::lround(distanceM)) +
              " m away, the snapping chooses there, " + off);
  }
  if (why)
    *why = off;
  return best->m_place;
}
}  // namespace nogps
