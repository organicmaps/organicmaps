#include "testing/testing.hpp"

#include "map/map_tests/nogps_test_roads.hpp"

#include "map/nogps/bend_matcher.hpp"
#include "map/nogps/geo.hpp"

#include "geometry/point2d.hpp"

#include "base/math.hpp"

#include <cmath>
#include <optional>
#include <vector>

namespace nogps_bend_matcher_tests
{
using namespace nogps;
using nogps_test::At;

/// One road given by its points, meters east and north.
class Road : public Roads
{
public:
  explicit Road(double startBearingDeg) : m_bearingDeg(startBearingDeg) { m_points.emplace_back(0, 0); }

  /// Continues the road turning it evenly by |turnDeg|, clockwise is positive.
  Road & Add(double lengthM, double turnDeg)
  {
    auto const steps = static_cast<int>(std::round(lengthM));
    for (int i = 0; i < steps; ++i)
    {
      m_bearingDeg += turnDeg / steps;
      auto const & last = m_points.back();
      m_points.emplace_back(last.x + std::sin(math::DegToRad(m_bearingDeg)),
                            last.y + std::cos(math::DegToRad(m_bearingDeg)));
    }
    return *this;
  }

  /// The road at the distance from its start.
  RoadPoint AtDistance(double distanceM) const
  {
    auto const index = static_cast<size_t>(std::round(distanceM));
    auto const & point = m_points[index];
    auto const & next = m_points[index + 1];
    return {At(point.x, point.y), math::RadToDeg(std::atan2(next.x - point.x, next.y - point.y))};
  }

  std::optional<RoadPoint> Snap(ms::LatLon const & point, std::optional<double>, double radiusM) override
  {
    double const x = nogps_test::East(point);
    double const y = nogps_test::North(point);
    std::optional<size_t> best;
    double bestDistance = radiusM;
    for (size_t i = 0; i + 1 < m_points.size(); ++i)
    {
      double const distance = std::hypot(m_points[i].x - x, m_points[i].y - y);
      if (distance <= bestDistance)
      {
        bestDistance = distance;
        best = i;
      }
    }
    if (!best)
      return {};
    return AtDistance(*best);
  }

  std::vector<ms::LatLon> FindCrossings(ms::LatLon const &, double) override { return {}; }

private:
  std::vector<m2::PointD> m_points;
  // The direction the road is being drawn in.
  double m_bearingDeg;
};

/// Drives the car along the road from its start to the distance.
/// \param laneChangeAtM where the car changes a lane: it turns by 5 degrees and back within 40 m.
BendMatcher Drive(Road const & road, double distanceM, std::optional<double> laneChangeAtM)
{
  BendMatcher matcher;
  double bearing = road.AtDistance(0).m_bearingDeg;
  for (int m = 1; m <= distanceM; ++m)
  {
    double const roadBearing = road.AtDistance(m).m_bearingDeg;
    double yaw = AngleDiff(roadBearing, bearing);
    bearing = roadBearing;
    if (laneChangeAtM && m > *laneChangeAtM && m <= *laneChangeAtM + 10)
      yaw += 0.5;
    else if (laneChangeAtM && m > *laneChangeAtM + 30 && m <= *laneChangeAtM + 40)
      yaw -= 0.5;
    matcher.OnMotion(yaw, 1);
  }
  return matcher;
}

std::optional<double> Match(BendMatcher const & matcher, Road & road, double positionM)
{
  auto const position = road.AtDistance(positionM);
  return matcher.Match(road, position.m_point, position.m_bearingDeg, 60);
}

// 300 m east, a bend of 40 degrees to the right 80 m long, 300 m straight again.
Road BentRoad()
{
  Road road(90);
  road.Add(300, 0).Add(80, 40).Add(300, 0);
  return road;
}

UNIT_TEST(NoGps_BendMatcher_FindsPositionAheadOfCar)
{
  auto road = BentRoad();
  // The car is 60 m after the bend, the calculated position is 25 m farther.
  auto const matcher = Drive(road, 440, {});
  auto const ahead = Match(matcher, road, 465);
  TEST(ahead, ());
  TEST_ALMOST_EQUAL_ABS(*ahead, 25.0, 3.0, ());
}

UNIT_TEST(NoGps_BendMatcher_FindsPositionBehindCar)
{
  auto road = BentRoad();
  auto const matcher = Drive(road, 440, {});
  auto const ahead = Match(matcher, road, 410);
  TEST(ahead, ());
  TEST_ALMOST_EQUAL_ABS(*ahead, -30.0, 3.0, ());
}

UNIT_TEST(NoGps_BendMatcher_FitsRightPosition)
{
  auto road = BentRoad();
  auto const matcher = Drive(road, 440, {});
  auto const ahead = Match(matcher, road, 440);
  TEST(ahead, ());
  TEST_ALMOST_EQUAL_ABS(*ahead, 0.0, 3.0, ());
}

UNIT_TEST(NoGps_BendMatcher_TellsNothingOnStraightRoad)
{
  Road road(90);
  road.Add(600, 0);
  auto const matcher = Drive(road, 400, {});
  TEST(!Match(matcher, road, 420), ());
}

UNIT_TEST(NoGps_BendMatcher_LaneChangeIsNotBend)
{
  // A lane is changed on a straight road: the car turns by 5 degrees and back.
  Road straight(90);
  straight.Add(600, 0);
  TEST(!Match(Drive(straight, 400, 300), straight, 420), ());
  // A lane changed on the way doesn't move the position found by the bend.
  auto road = BentRoad();
  auto const ahead = Match(Drive(road, 440, 395), road, 465);
  TEST(ahead, ());
  TEST_ALMOST_EQUAL_ABS(*ahead, 25.0, 5.0, ());
}

UNIT_TEST(NoGps_BendMatcher_TellsNothingOnEvenBend)
{
  // A roundabout or a long even bend looks the same wherever the car is on it.
  Road road(90);
  road.Add(100, 0).Add(500, 300);
  auto const matcher = Drive(road, 400, {});
  TEST(!Match(matcher, road, 420), ());
}

UNIT_TEST(NoGps_BendMatcher_TellsNothingBeforeEnoughIsDriven)
{
  auto road = BentRoad();
  BendMatcher matcher;
  matcher.OnMotion(0, 50);
  TEST(!Match(matcher, road, 400), ());
}

UNIT_TEST(NoGps_BendMatcher_TellsNothingWhenErrorIsLarger)
{
  auto road = BentRoad();
  // The position is 100 m ahead: the bend is not within the searched 60 m, the road around looks straight.
  auto const matcher = Drive(road, 440, {});
  TEST(!Match(matcher, road, 540), ());
}
}  // namespace nogps_bend_matcher_tests
