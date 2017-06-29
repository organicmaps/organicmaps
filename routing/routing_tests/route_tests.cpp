#include "testing/testing.hpp"

#include "routing/route.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/turns.hpp"

#include "routing/base/followed_polyline.hpp"

#include "platform/location.hpp"

#include "geometry/point2d.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

using namespace routing;
using namespace routing::turns;

namespace
{
static vector<m2::PointD> const kTestGeometry({{0, 0}, {0,1}, {1,1}, {1,2}, {1,3}});
static vector<Segment> const kTestSegments(
    {{0, 0, 0, true}, {0, 0, 1, true}, {0, 0, 2, true}, {0, 0, 3, true}});
static Route::TTurns const kTestTurns(
    {turns::TurnItem(1, turns::TurnDirection::TurnLeft),
     turns::TurnItem(2, turns::TurnDirection::TurnRight),
     turns::TurnItem(4, turns::TurnDirection::ReachedYourDestination)});
static Route::TStreets const kTestNames({{0, "Street1"}, {1, "Street2"}, {4, "Street3"}});
static Route::TTimes const kTestTimes({Route::TTimeItem(1, 5), Route::TTimeItem(3, 10),
                                      Route::TTimeItem(4, 15)});

static Route::TTurns const kTestTurns2(
    {turns::TurnItem(0, turns::TurnDirection::NoTurn),
     turns::TurnItem(1, turns::TurnDirection::TurnLeft),
     turns::TurnItem(2, turns::TurnDirection::TurnRight),
     turns::TurnItem(3, turns::TurnDirection::NoTurn),
     turns::TurnItem(4, turns::TurnDirection::ReachedYourDestination)});
static vector<string> const kTestNames2 = {"Street0", "Street1", "Street2", "", "Street3"};
static vector<double> const kTestTimes2 = {0.0, 5.0, 6.0, 10.0, 15.0};

void GetTestRouteSegments(vector<m2::PointD> const & routePoints, Route::TTurns const & turns,
                          vector<string> const & streets, vector<double> const & times,
                          vector<RouteSegment> & routeSegments)
{
  CHECK_EQUAL(routePoints.size(), turns.size(), ());
  CHECK_EQUAL(turns.size(), streets.size(), ());
  CHECK_EQUAL(turns.size(), times.size(), ());

  FollowedPolyline poly(routePoints.cbegin(), routePoints.cend());

  double routeLengthMeters = 0.0;
  double routeLengthMertc = 0.0;
  for (size_t i = 1; i < routePoints.size(); ++i)
  {
    routeLengthMeters += MercatorBounds::DistanceOnEarth(routePoints[i - 1], routePoints[i]);
    routeLengthMertc += routePoints[i - 1].Length(routePoints[i]);
    routeSegments.emplace_back(
        Segment(0 /* mwm id */, static_cast<uint32_t>(i) /* feature id */, 0 /* seg id */, true /* forward */), turns[i],
        Junction(routePoints[i], feature::kInvalidAltitude), streets[i], routeLengthMeters,
        routeLengthMertc, times[i], traffic::SpeedGroup::Unknown);
  }
}

location::GpsInfo GetGps(double x, double y)
{
  location::GpsInfo info;
  info.m_latitude = MercatorBounds::YToLat(y);
  info.m_longitude = MercatorBounds::XToLon(x);
  info.m_horizontalAccuracy = 2;
  info.m_speed = -1;
  return info;
}
}  // namespace

UNIT_TEST(AddAdsentCountryToRouteTest)
{
  Route route("TestRouter");
  route.AddAbsentCountry("A");
  route.AddAbsentCountry("A");
  route.AddAbsentCountry("B");
  route.AddAbsentCountry("C");
  route.AddAbsentCountry("B");
  set<string> const & absent = route.GetAbsentCountries();
  TEST(absent.find("A") != absent.end(), ());
  TEST(absent.find("B") != absent.end(), ());
  TEST(absent.find("C") != absent.end(), ());
}

UNIT_TEST(DistanceToCurrentTurnTest)
{
  Route route("TestRouter");
  vector<RouteSegment> routeSegments;
  GetTestRouteSegments(kTestGeometry, kTestTurns2, kTestNames2, kTestTimes2, routeSegments);
  route.SetGeometry(kTestGeometry.begin(), kTestGeometry.end());
  vector<turns::TurnItem> turns(kTestTurns);

  route.SetRouteSegments(routeSegments);

  double distance;
  turns::TurnItem turn;

  route.GetCurrentTurn(distance, turn);
  TEST(my::AlmostEqualAbs(distance,
                          MercatorBounds::DistanceOnEarth(kTestGeometry[0], kTestGeometry[1]), 0.1),
                          ());
  TEST_EQUAL(turn, kTestTurns[0], ());

  route.MoveIterator(GetGps(0, 0.5));
  route.GetCurrentTurn(distance, turn);
  TEST(my::AlmostEqualAbs(distance,
                          MercatorBounds::DistanceOnEarth({0, 0.5}, kTestGeometry[1]), 0.1), ());
  TEST_EQUAL(turn, kTestTurns[0], ());

  route.MoveIterator(GetGps(1, 1.5));
  route.GetCurrentTurn(distance, turn);
  TEST(my::AlmostEqualAbs(distance,
                          MercatorBounds::DistanceOnEarth({1, 1.5}, kTestGeometry[4]), 0.1), ());
  TEST_EQUAL(turn, kTestTurns[2], ());

  route.MoveIterator(GetGps(1, 2.5));
  route.GetCurrentTurn(distance, turn);
  TEST(my::AlmostEqualAbs(distance,
                          MercatorBounds::DistanceOnEarth({1, 2.5}, kTestGeometry[4]), 0.1), ());
  TEST_EQUAL(turn, kTestTurns[2], ());
}

UNIT_TEST(NextTurnTest)
{
  Route route("TestRouter");
  vector<RouteSegment> routeSegments;
  GetTestRouteSegments(kTestGeometry, kTestTurns2, kTestNames2, kTestTimes2, routeSegments);
  route.SetRouteSegments(routeSegments);
  route.SetGeometry(kTestGeometry.begin(), kTestGeometry.end());

  double distance, nextDistance;
  turns::TurnItem turn;
  turns::TurnItem nextTurn;

  route.GetCurrentTurn(distance, turn);
  route.GetNextTurn(nextDistance, nextTurn);
  TEST_EQUAL(turn, kTestTurns[0], ());
  TEST_EQUAL(nextTurn, kTestTurns[1], ());

  route.MoveIterator(GetGps(0.5, 1));
  route.GetCurrentTurn(distance, turn);
  route.GetNextTurn(nextDistance, nextTurn);
  TEST_EQUAL(turn, kTestTurns[1], ());
  TEST_EQUAL(nextTurn, kTestTurns[2], ());

  route.MoveIterator(GetGps(1, 1.5));
  route.GetCurrentTurn(distance, turn);
  route.GetNextTurn(nextDistance, nextTurn);
  TEST_EQUAL(turn, kTestTurns[2], ());
  TEST_EQUAL(nextTurn, turns::TurnItem(), ());
}

UNIT_TEST(NextTurnsTest)
{
  Route route("TestRouter");
  route.SetGeometry(kTestGeometry.begin(), kTestGeometry.end());
  vector<RouteSegment> routeSegments;
  GetTestRouteSegments(kTestGeometry, kTestTurns2, kTestNames2, kTestTimes2, routeSegments);
  route.SetRouteSegments(routeSegments);

  vector<turns::TurnItem> turns(kTestTurns);
  vector<turns::TurnItemDist> turnsDist;

  {
    TEST(route.GetNextTurns(turnsDist), ());
    TEST_EQUAL(turnsDist.size(), 2, ());
    double const firstSegLenM = MercatorBounds::DistanceOnEarth(kTestGeometry[0], kTestGeometry[1]);
    double const secondSegLenM = MercatorBounds::DistanceOnEarth(kTestGeometry[1], kTestGeometry[2]);
    TEST(my::AlmostEqualAbs(turnsDist[0].m_distMeters, firstSegLenM, 0.1), ());
    TEST(my::AlmostEqualAbs(turnsDist[1].m_distMeters, firstSegLenM + secondSegLenM, 0.1), ());
    TEST_EQUAL(turnsDist[0].m_turnItem, kTestTurns[0], ());
    TEST_EQUAL(turnsDist[1].m_turnItem, kTestTurns[1], ());
  }
  {
    double const x = 0.;
    double const y = 0.5;
    route.MoveIterator(GetGps(x, y));
    TEST(route.GetNextTurns(turnsDist), ());
    TEST_EQUAL(turnsDist.size(), 2, ());
    double const firstSegLenM = MercatorBounds::DistanceOnEarth({x, y}, kTestGeometry[1]);
    double const secondSegLenM = MercatorBounds::DistanceOnEarth(kTestGeometry[1], kTestGeometry[2]);
    TEST(my::AlmostEqualAbs(turnsDist[0].m_distMeters, firstSegLenM, 0.1), ());
    TEST(my::AlmostEqualAbs(turnsDist[1].m_distMeters, firstSegLenM + secondSegLenM, 0.1), ());
    TEST_EQUAL(turnsDist[0].m_turnItem, kTestTurns[0], ());
    TEST_EQUAL(turnsDist[1].m_turnItem, kTestTurns[1], ());
  }
  {
    double const x = 1.;
    double const y = 2.5;
    route.MoveIterator(GetGps(x, y));
    TEST(route.GetNextTurns(turnsDist), ());
    TEST_EQUAL(turnsDist.size(), 1, ());
    double const firstSegLenM = MercatorBounds::DistanceOnEarth({x, y}, kTestGeometry[4]);
    TEST(my::AlmostEqualAbs(turnsDist[0].m_distMeters, firstSegLenM, 0.1), ());
    TEST_EQUAL(turnsDist[0].m_turnItem, kTestTurns[2], ());
  }
  {
    double const x = 1.;
    double const y = 3.5;
    route.MoveIterator(GetGps(x, y));
    TEST(route.GetNextTurns(turnsDist), ());
    TEST_EQUAL(turnsDist.size(), 1, ());
  }
}

UNIT_TEST(RouteNameTest)
{
  Route route("TestRouter");

  route.SetGeometry(kTestGeometry.begin(), kTestGeometry.end());
  vector<RouteSegment> routeSegments;
  GetTestRouteSegments(kTestGeometry, kTestTurns2, kTestNames2, kTestTimes2, routeSegments);
  route.SetRouteSegments(routeSegments);

  string name;
  route.GetCurrentStreetName(name);
  TEST_EQUAL(name, "Street1", ());

  route.GetStreetNameAfterIdx(0, name);
  TEST_EQUAL(name, "Street1", ());

  route.GetStreetNameAfterIdx(1, name);
  TEST_EQUAL(name, "Street1", ());

  route.GetStreetNameAfterIdx(2, name);
  TEST_EQUAL(name, "Street2", ());

  route.GetStreetNameAfterIdx(3, name);
  TEST_EQUAL(name, "Street3", ());

  route.GetStreetNameAfterIdx(4, name);
  TEST_EQUAL(name, "Street3", ());

  location::GpsInfo info;
  info.m_longitude = 1.0;
  info.m_latitude = 2.0;
  route.MoveIterator(info);
  route.GetCurrentStreetName(name);
  TEST_EQUAL(name, "Street3", ());
}
