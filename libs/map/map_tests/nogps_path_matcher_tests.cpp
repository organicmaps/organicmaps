#include "testing/testing.hpp"

#include "map/map_tests/nogps_test_roads.hpp"

#include "map/nogps/geo.hpp"
#include "map/nogps/path_matcher.hpp"

#include "base/math.hpp"

#include <cmath>
#include <string>
#include <vector>

namespace nogps_path_matcher_tests
{
using namespace nogps;
using namespace nogps_test;

double constexpr kRampRadiusM = 20;

// A ramp leaving the street going east at |eastM| and looping to the right onto the street 40 m south of it, which
// goes west.
void AddRamp(std::vector<PieceRoads::Piece> & pieces, double eastM)
{
  for (int i = 0; i < 12; ++i)
  {
    double const from = math::DegToRad(15.0 * i);
    double const to = math::DegToRad(15.0 * (i + 1));
    pieces.push_back({eastM + kRampRadiusM * std::sin(from), -kRampRadiusM + kRampRadiusM * std::cos(from),
                      eastM + kRampRadiusM * std::sin(to), -kRampRadiusM + kRampRadiusM * std::cos(to)});
  }
}

// The street going east, the ramp at 40 m, the street it leads to, and a street going west 15 m south of that one.
std::vector<PieceRoads::Piece> Streets()
{
  std::vector<PieceRoads::Piece> pieces = {{-300, 0, 300, 0}, {40, -40, -300, -40}, {300, -55, -300, -55}};
  AddRamp(pieces, 40);
  return pieces;
}

// The car drives 100 m east to the ramp, loops along it and drives 40 m west: it is at (0, -40).
PathMatcher DriveLoop()
{
  PathMatcher matcher;
  double headingDeg = 90;
  for (int i = 0; i < 100; ++i)
    matcher.OnMotion(0, 1, headingDeg);
  double const loopM = math::pi * kRampRadiusM;
  for (int i = 0; i < 100; ++i)
  {
    headingDeg += 1.8;
    matcher.OnMotion(1.8, loopM / 100, headingDeg);
  }
  for (int i = 0; i < 40; ++i)
    matcher.OnMotion(0, 1, headingDeg);
  return matcher;
}

UNIT_TEST(NoGps_PathMatcher_FindsLoopStartedFromWrongPlace)
{
  PieceRoads roads(Streets(), {});
  auto const matcher = DriveLoop();
  // The loop was started 25 m too far east and 15 m too far south: it has been drawn beside the ramp, and the car
  // is by the calculation on the street 15 m south of the right one.
  std::string whyNot;
  auto const place = matcher.Match(roads, At(25, -55), &whyNot);
  TEST(place, (whyNot));
  TEST_LESS_OR_EQUAL(Distance(place->m_point, At(0, -40)), 6.0, (East(place->m_point), North(place->m_point)));
  TEST_ALMOST_EQUAL_ABS(place->m_bearingDeg, 270.0, 1.0, ());
}

UNIT_TEST(NoGps_PathMatcher_TurnsWayAsCarLooksNow)
{
  // The car drove 150 m east looking north by the calculation, then the user marked it looking east.
  PieceRoads roads(Streets(), {});
  PathMatcher matcher;
  for (int i = 0; i < 150; ++i)
    matcher.OnMotion(0, 1, 0);
  TEST_GREATER_OR_EQUAL(*matcher.CountMissesAt(roads, At(100, 0)), PathMatcher::kMinMissesToMove, ());
  matcher.SetHeading(90);
  TEST_EQUAL(*matcher.CountMissesAt(roads, At(100, 0)), 0, ());
}

UNIT_TEST(NoGps_PathMatcher_KeepsCarWhereWayLiesOnRoads)
{
  PieceRoads roads(Streets(), {});
  std::string whyNot;
  TEST(!DriveLoop().Match(roads, At(0, -40), &whyNot), ());
  TEST(whyNot.starts_with("the way driven lies on the roads, 0 of "), (whyNot));
}

UNIT_TEST(NoGps_PathMatcher_LeavesNearPlacesToSnapping)
{
  // The loop was started 12 m too far east: it misses the ramp, but the place on the roads is 12 m away.
  PieceRoads roads(Streets(), {});
  std::string whyNot;
  TEST(!DriveLoop().Match(roads, At(12, -40), &whyNot), ());
  TEST(whyNot.find("m away, the snapping chooses there") != std::string::npos, (whyNot));
}

UNIT_TEST(NoGps_PathMatcher_DoesNotGuessAmongPlaces)
{
  // A second ramp 40 m farther east leads to the same street: the way lies on the roads at both.
  auto pieces = Streets();
  pieces.push_back({80, -40, 40, -40});
  AddRamp(pieces, 80);
  PieceRoads roads(pieces, {});
  std::string whyNot;
  TEST(!DriveLoop().Match(roads, At(25, -55), &whyNot), ());
  TEST(whyNot.find("in places") != std::string::npos, (whyNot));
}

UNIT_TEST(NoGps_PathMatcher_NeedsLongWay)
{
  PieceRoads roads(Streets(), {});
  PathMatcher matcher;
  for (int i = 0; i < 50; ++i)
    matcher.OnMotion(0, 1, 90);
  std::string whyNot;
  TEST(!matcher.Match(roads, At(25, -55), &whyNot), ());
  TEST_EQUAL(whyNot, "too short a way driven", ());
}
}  // namespace nogps_path_matcher_tests
