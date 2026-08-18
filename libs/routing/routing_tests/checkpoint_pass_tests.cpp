#include "testing/testing.hpp"

#include "routing/routing_tests/index_graph_tools.hpp"
#include "routing/routing_tests/tools.hpp"

#include "routing/fake_feature_ids.hpp"
#include "routing/route.hpp"
#include "routing/router.hpp"
#include "routing/routing_callbacks.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/routing_session.hpp"
#include "routing/routing_settings.hpp"

#include "traffic/traffic_cache.hpp"

#include "indexer/classificator_loader.hpp"

#include "platform/location.hpp"
#include "platform/platform.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace checkpoint_pass_tests
{
using namespace routing;
using namespace std;

double constexpr kDegToM = 111317.0;  // metres per mercator degree at the equator.
double constexpr kAx = 0.005;         // x of A, the projection of the intermediate stop onto the road.

location::GpsInfo GetGps(double x, double y, double accuracy = 5.0, double speed = 20.0)
{
  location::GpsInfo info;
  info.m_latitude = mercator::YToLat(y);
  info.m_longitude = mercator::XToLon(x);
  info.m_horizontalAccuracy = accuracy;
  info.m_speed = speed;
  return info;
}

Segment RealSeg(uint32_t idx)
{
  return {0, 0, idx, true};
}

Segment FakeSeg(uint32_t idx)
{
  return {kFakeNumMwmId, FakeFeatureIds::kIndexGraphStarterId, idx, true};
}

// Production-faithful two-leg route. Road along the equator; A = (0.005, 0); the intermediate
// stop B sits |connectorM| north of A. Polyline (per IndexGraphStarter::AddEnding + RedressRoute):
//   start, start, road..., A, B, B, | B, A, road..., finish, finish
// Leg 0 tail = pure fakes (A->B, B->B); leg 1 head = pure fakes (B->B, B->A).
// |reversed| turns leg 1 back west instead, so that it retraces the road of leg 0.
struct Fixture
{
  explicit Fixture(double connectorM, VehicleType vehicle = VehicleType::Car, bool reversed = false)
    : m_b(kAx, connectorM / kDegToM)
  {
    m2::PointD const a(kAx, 0.0);
    m_path = {{0.0, 0.0}, {0.0, 0.0}, {0.001, 0.0}, {0.002, 0.0}, {0.003, 0.0}, {0.004, 0.0}, a, m_b};
    m_stopIdx = m_path.size();  // polyline index of leg-0 end vertex (the standalone B).
    m_path.push_back(m_b);      // leg 0 standalone B->B end vertex.
    m_path.push_back(m_b);      // leg 1 standalone B->B vertex.
    if (reversed)
      m_path.insert(m_path.end(), {a, {0.004, 0.0}, {0.003, 0.0}, {0.002, 0.0}, {0.001, 0.0}, {0.0, 0.0}, {0.0, 0.0}});
    else
      m_path.insert(m_path.end(),
                    {a, {0.006, 0.0}, {0.007, 0.0}, {0.008, 0.0}, {0.009, 0.0}, {0.01, 0.0}, {0.01, 0.0}});

    vector<Segment> segs;
    for (uint32_t i = 0; i + 1 < m_path.size(); ++i)
      segs.push_back(RealSeg(i));
    segs[m_stopIdx - 2] = FakeSeg(m_stopIdx - 2);  // projection edge A->B
    segs[m_stopIdx - 1] = FakeSeg(m_stopIdx - 1);  // standalone B->B
    segs[m_stopIdx] = FakeSeg(m_stopIdx);          // leg 1 standalone B->B
    segs[m_stopIdx + 1] = FakeSeg(m_stopIdx + 1);  // leg 1 projection edge B->A

    vector<RouteSegment> segments;
    RouteSegmentsFrom(segs, m_path, {}, {}, segments);
    FillSegmentInfo(vector<double>(m_path.size() - 1, 0.0), segments);

    m_route.SetRoutingSettings(GetRoutingSettings(vehicle));
    m_route.SetRouteSegments(std::move(segments));
    m_route.SetGeometry(m_path.begin(), m_path.end());

    SetSubroutes(m_b);
  }

  // |leg0Finish| is what SubrouteAttrs reports as the first leg's checkpoint. Every producer fills
  // it with B, except IndexRouter::AdjustRoute(), which reports the route's final destination for
  // the leg it rebuilds -- hence the parameter.
  void SetSubroutes(m2::PointD const & leg0Finish)
  {
    auto const wa = [](m2::PointD const & p)
    { return geometry::PointWithAltitude(p, geometry::kDefaultAltitudeMeters); };
    m_route.SetSubroutes(
        vector<Route::SubrouteAttrs>{Route::SubrouteAttrs(wa(m_path.front()), wa(leg0Finish), 0, m_stopIdx),
                                     Route::SubrouteAttrs(wa(m_b), wa(m_path.back()), m_stopIdx, m_path.size() - 1)});
  }

  double RemainingToStopM() const
  {
    auto const & segments = m_route.GetRouteSegments();
    return segments[m_stopIdx - 1].GetDistFromBeginningMeters() - m_route.GetCurrentDistanceFromBeginMeters();
  }

  m2::PointD m_b;
  vector<m2::PointD> m_path;
  size_t m_stopIdx = 0;
  Route m_route;
};

double MetersPastA(m2::PointD const & pos)
{
  return (pos.x - kAx) * kDegToM;
}

// The router-side guard for the invariant Route::GetSubrouteEndConnectorMeters() relies on: every
// ending built by IndexGraphStarter::AddEnding() contributes exactly two trailing pure fake
// segments -- the projection edge A->B and the zero-length standalone B->B -- whose summed length
// is |A - B|. Fails if the shape of a route ending ever changes.
UNIT_TEST(PassCheckpoint_EndingTailInvariant)
{
  using namespace routing_test;
  using AlgoT = AStarAlgorithm<Segment, SegmentEdge, RouteWeight>;

  classificator::Load();  // CreateEstimatorForCar reads hwtag types.

  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();
  loader->AddRoad(0 /* featureId */, false /* oneWay */, 50.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {3.0, 0.0}}));

  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimatorForCar(trafficCache);
  unique_ptr<WorldGraph> worldGraph = BuildWorldGraph(std::move(loader), estimator, vector<Joint>());

  for (double offY : {0.0, 0.0003, 0.002})  // on-road, ~33 m and ~223 m off-road checkpoints.
  {
    m2::PointD const b(1.5, offY);
    m2::PointD const a(1.5, 0.0);
    auto const start = MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(0.5, 0.0), *worldGraph);
    auto const finish = MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, b, *worldGraph);
    auto starter = MakeStarter(start, finish, *worldGraph);

    vector<Segment> route;
    double timeSec;
    TEST_EQUAL(CalculateRoute(*starter, route, timeSec), AlgoT::Result::OK, ("offY =", offY));
    size_t const n = route.size();
    TEST_GREATER_OR_EQUAL(n, 3, ("offY =", offY));

    size_t pureFakeTail = 0;
    for (auto rit = route.rbegin(); rit != route.rend(); ++rit)
    {
      Segment real = *rit;
      if (starter->ConvertToReal(real))
        break;
      ++pureFakeTail;
    }
    TEST_EQUAL(pureFakeTail, 2, ("Ending must contribute exactly two trailing pure fakes, offY =", offY));

    // The standalone B->B is zero-length and sits at the checkpoint.
    auto const & lastFrom = starter->GetJunction(route[n - 1], false /* front */);
    auto const & lastTo = starter->GetJunction(route[n - 1], true /* front */);
    TEST_EQUAL(lastFrom, lastTo, ("offY =", offY));
    TEST_ALMOST_EQUAL_ABS(mercator::FromLatLon(lastTo.GetLatLon()).y, b.y, 1e-9, ("offY =", offY));

    // The projection edge runs A->B, where A is the perpendicular foot of B on the road.
    auto const projFrom = mercator::FromLatLon(starter->GetJunction(route[n - 2], false /* front */).GetLatLon());
    auto const projTo = mercator::FromLatLon(starter->GetJunction(route[n - 2], true /* front */).GetLatLon());
    TEST_ALMOST_EQUAL_ABS(projFrom.x, a.x, 1e-9, ("offY =", offY));
    TEST_ALMOST_EQUAL_ABS(projFrom.y, a.y, 1e-9, ("offY =", offY));
    TEST_ALMOST_EQUAL_ABS(projTo.y, b.y, 1e-9, ("offY =", offY));

    // The summed tail length is |A - B|, i.e. the remaining-distance plateau of the subroute.
    double const tailLenM = ms::DistanceOnEarth(starter->GetJunction(route[n - 2], false /* front */).GetLatLon(),
                                                starter->GetJunction(route[n - 2], true /* front */).GetLatLon()) +
                            ms::DistanceOnEarth(lastFrom.GetLatLon(), lastTo.GetLatLon());
    TEST_ALMOST_EQUAL_ABS(tailLenM, mercator::DistanceOnEarth(a, b), 1e-6, ("offY =", offY));
  }
}

// The derived connector (Dist(E-1) - Dist(E-3)) is exactly the remaining-distance plateau the
// matcher saturates at when driving past an off-road stop.
UNIT_TEST(PassCheckpoint_ConnectorEqualsPlateau)
{
  for (double c : {10.0, 15.0, 33.0, 50.0, 100.0, 200.0})
  {
    Fixture f(c, VehicleType::Car);
    double plateauM = -1.0;
    for (double x = 0.0035; x < 0.0075; x += 0.00005)
      if (f.m_route.MoveIterator(GetGps(x, 0.0)) && MetersPastA({x, 0.0}) > 5.0)
        plateauM = f.RemainingToStopM();

    TEST_GREATER(plateauM, 0.0, ("No matched fix past A, c =", c));

    auto const & segments = f.m_route.GetRouteSegments();
    double const derivedM =
        segments[f.m_stopIdx - 1].GetDistFromBeginningMeters() - segments[f.m_stopIdx - 3].GetDistFromBeginningMeters();
    TEST_ALMOST_EQUAL_ABS(derivedM, plateauM, 1e-6, ("Derived connector must equal the plateau, c =", c));
    TEST_ALMOST_EQUAL_ABS(derivedM, c, 0.01, ("Nominal degree-to-metre conversion tolerance, c =", c));
  }
}

// Driving past an off-road stop passes it promptly after the closest point of approach A, for
// every vehicle and every off-road distance.
UNIT_TEST(PassCheckpoint_OffRoadPassBy)
{
  for (auto const vehicle : {VehicleType::Car, VehicleType::Bicycle, VehicleType::Pedestrian, VehicleType::Transit})
  {
    // c = 10 is the kOnEndToleranceM boundary: kDegToM slightly understates the equator degree, so
    // the fixture's derived connector lands just above 10 m and takes the off-road branch.
    for (double c : {10.0, 15.0, 33.0, 50.0, 100.0, 200.0})
    {
      Fixture f(c, vehicle);
      double fireAtM = -1.0;
      for (double x = 0.0035; x < 0.0075 && fireAtM < 0.0; x += 0.00005)
        if (f.m_route.MoveIterator(GetGps(x, 0.0)) && f.m_route.IsSubroutePassed(0))
          fireAtM = MetersPastA({x, 0.0});

      TEST_GREATER_OR_EQUAL(fireAtM, 14.9, ("Must not fire before departing A", vehicle, c));
      TEST_LESS_OR_EQUAL(fireAtM, 25.0, ("Must fire promptly after A", vehicle, c));
    }
  }
}

// Pedestrian is the tightest case: the gate fires ~15 m past A while matching dies at 20 m.
// At a realistic 1 Hz walking cadence there must be matched fixes to spare.
UNIT_TEST(PassCheckpoint_PedestrianFixSpacing)
{
  for (double c : {15.0, 33.0, 100.0})
  {
    Fixture f(c, VehicleType::Pedestrian);
    size_t matchedFromFire = 0;
    for (double x = 0.0045; x < 0.0055; x += 1.4 / kDegToM)
      if (f.m_route.MoveIterator(GetGps(x, 0.0, 5.0 /* accuracy */, 1.4 /* speed */)) && f.m_route.IsSubroutePassed(0))
        ++matchedFromFire;

    TEST_GREATER_OR_EQUAL(matchedFromFire, 3, ("Pedestrian needs the firing fix and >= 2 more, c =", c));
  }
}

// On-road stops (connector < kOnEndToleranceM) keep today's rule bit-for-bit.
UNIT_TEST(PassCheckpoint_OnRoadRegression)
{
  for (double c : {0.0, 2.0, 5.0, 8.0, 9.0})
  {
    Fixture f(c, VehicleType::Car);
    int fireOld = -1, fireNew = -1, sample = 0;
    for (double x = 0.0035; x < 0.0075; x += 0.00005, ++sample)
    {
      if (!f.m_route.MoveIterator(GetGps(x, 0.0)))
        continue;
      if (fireOld < 0 && f.RemainingToStopM() < 10.0)
        fireOld = sample;
      if (fireNew < 0 && f.m_route.IsSubroutePassed(0))
        fireNew = sample;
    }
    TEST_GREATER_OR_EQUAL(fireNew, 0, ("The stop must be passed, c =", c));
    TEST_EQUAL(fireOld, fireNew, ("On-road stop behaviour must be identical, c =", c));
  }
}

// A user who actually visits the stop keeps it through approach, drive-in, dwell and moving around
// behind the stop; it is passed only after departing along the road.
UNIT_TEST(PassCheckpoint_VisitorKeepsStop)
{
  // Returns the track phase in which the stop was passed and how far past A that happened.
  auto const runVisit = [](double c, bool wrongAttrsFinish)
  {
    Fixture f(c, VehicleType::Car);
    if (wrongAttrsFinish)
      f.SetSubroutes(f.m_path.back());

    vector<pair<string, m2::PointD>> track;
    for (double x = 0.0045; x < kAx; x += 0.00005)
      track.emplace_back("approach-road", m2::PointD(x, 0.0));
    for (double t = 0.0; t <= 1.0; t += 0.1)
      track.emplace_back("drive-in", m2::PointD(kAx, c / kDegToM * t));
    for (int i = 0; i < 5; ++i)
      track.emplace_back("dwell-at-B", f.m_b);
    // Moving around behind the stop (parking, another entrance) leaves the projection at B, so the
    // depart gate opens and only the "at/heading into the stop" check keeps the checkpoint.
    for (int i = 0; i < 2; ++i)
      track.emplace_back("behind-B", m2::PointD(f.m_b.x, f.m_b.y + 20.0 / kDegToM));
    for (double t = 1.0; t >= 0.0; t -= 0.1)
      track.emplace_back("drive-out", m2::PointD(kAx, c / kDegToM * t));
    for (double x = kAx + 0.00005; x < 0.0062; x += 0.00005)
      track.emplace_back("depart-east", m2::PointD(x, 0.0));

    pair<string, double> fire{"never", -1.0};
    for (auto const & [phase, pos] : track)
    {
      // Dwell fixes report zero speed: the geometry conditions, not the speed gate, must be what
      // protects the visitor.
      double const speed = phase == "dwell-at-B" ? 0.0 : 20.0;
      if (!f.m_route.MoveIterator(GetGps(pos.x, pos.y, 5.0 /* accuracy */, speed)) || fire.first != "never")
        continue;
      if (f.m_route.IsSubroutePassed(0))
        fire = {phase, MetersPastA(pos)};
    }
    return fire;
  };

  for (double c : {33.0, 100.0})
  {
    auto const [phase, fireAtM] = runVisit(c, false /* wrongAttrsFinish */);
    TEST_EQUAL(phase, "depart-east", ("Visitor must keep the stop until departing, c =", c));
    TEST_LESS_OR_EQUAL(fireAtM, 25.0, ("c =", c));

    // The checkpoint must be read from the route geometry, not from SubrouteAttrs: for a leg
    // rebuilt by IndexRouter::AdjustRoute() the latter holds the route's final destination.
    TEST_EQUAL(runVisit(c, true /* wrongAttrsFinish */).first, "depart-east",
               ("Wrong SubrouteAttrs finish must not drop the stop, c =", c));
  }
}

// Overshooting A by less than kCheckpointDepartM and turning back keeps the stop; a clear
// overshoot passes it (the documented boundary of the U-turn grace window).
UNIT_TEST(PassCheckpoint_UTurnGrace)
{
  auto const runOvershoot = [](double c, double overshootM)
  {
    Fixture f(c, VehicleType::Car);
    bool fired = false;
    auto const step = [&f, &fired](double x, double y)
    {
      if (f.m_route.MoveIterator(GetGps(x, y)))
        fired = fired || f.m_route.IsSubroutePassed(0);
    };

    for (double x = 0.0045; x <= kAx + overshootM / kDegToM + 1e-9; x += 0.00005)
      step(x, 0.0);
    // Turn back to A and drive into the stop.
    for (double x = kAx + overshootM / kDegToM; x >= kAx; x -= 0.00005)
      step(x, 0.0);
    for (double t = 0.0; t <= 1.0; t += 0.1)
      step(kAx, c / kDegToM * t);
    return fired;
  };

  for (double c : {33.0, 100.0})
  {
    TEST(!runOvershoot(c, 11.0), ("Overshoot below kCheckpointDepartM must keep the stop, c =", c));
    TEST(runOvershoot(c, 25.0), ("Overshoot beyond the depart distance passes the stop, c =", c));
  }
}

// A realistic constant lateral GPS error must not fake a departure while still approaching A.
UNIT_TEST(PassCheckpoint_ApproachLateralError)
{
  for (double c : {33.0, 100.0})
  {
    for (double side : {1.0, -1.0})
    {
      Fixture f(c, VehicleType::Car);
      for (double x = 0.0035; x < kAx - 1e-9; x += 0.00005)
      {
        if (!f.m_route.MoveIterator(GetGps(x, side * 10.0 / kDegToM)))
          continue;
        TEST(!f.m_route.IsSubroutePassed(0), ("10 m lateral error must not fake a pass, c =", c, "side =", side));
      }
    }
  }
}

// Standstill GPS wander satisfies every distance condition; only the car speed gate rejects it.
// A fix without speed data must not be blocked forever.
UNIT_TEST(PassCheckpoint_StandstillSpeedGate)
{
  double constexpr c = 33.0;
  m2::PointD const wander(kAx + 16.0 / kDegToM, -16.0 / kDegToM);
  {
    Fixture f(c, VehicleType::Car);
    TEST(f.m_route.MoveIterator(GetGps(0.0049, 0.0)), ());
    for (int i = 0; i < 5; ++i)
      if (f.m_route.MoveIterator(GetGps(wander.x, wander.y, 5.0 /* accuracy */, 0.2 /* speed */)))
        TEST(!f.m_route.IsSubroutePassed(0), ("Standstill wander must not pass the stop"));

    TEST(f.m_route.MoveIterator(GetGps(wander.x, wander.y, 5.0 /* accuracy */, 20.0 /* speed */)), ());
    TEST(f.m_route.IsSubroutePassed(0), ("A moving fix past A must pass the stop"));
  }
  {
    Fixture f(c, VehicleType::Car);
    TEST(f.m_route.MoveIterator(GetGps(0.0049, 0.0)), ());
    TEST(f.m_route.MoveIterator(GetGps(wander.x, wander.y, 5.0 /* accuracy */, -1.0 /* speed */)), ());
    TEST(f.m_route.IsSubroutePassed(0), ("Speedless fixes must not block passing forever"));
  }
}

// Production-faithful three-leg route: stops 33 m off the road at x = 0.005 and 15 m off it at
// x = 0.007, same tail/head shape as Fixture.
struct TwoStopFixture
{
  TwoStopFixture() : m_a1(0.005, 0.0), m_b1(0.005, 33.0 / kDegToM), m_a2(0.007, 0.0), m_b2(0.007, 15.0 / kDegToM)
  {
    m_path = {{0.0, 0.0}, {0.0, 0.0}, {0.002, 0.0}, {0.004, 0.0}, m_a1, m_b1};
    m_stop1Idx = m_path.size();
    m_path.push_back(m_b1);
    m_path.push_back(m_b1);
    m_path.insert(m_path.end(), {m_a1, {0.006, 0.0}, m_a2, m_b2});
    m_stop2Idx = m_path.size();
    m_path.push_back(m_b2);
    m_path.push_back(m_b2);
    m_path.insert(m_path.end(), {m_a2, {0.008, 0.0}, {0.009, 0.0}, {0.009, 0.0}});

    vector<Segment> segs;
    for (uint32_t i = 0; i + 1 < m_path.size(); ++i)
      segs.push_back(RealSeg(i));
    for (size_t idx : {m_stop1Idx - 2, m_stop1Idx - 1, m_stop1Idx, m_stop1Idx + 1, m_stop2Idx - 2, m_stop2Idx - 1,
                       m_stop2Idx, m_stop2Idx + 1})
      segs[idx] = FakeSeg(static_cast<uint32_t>(idx));

    vector<RouteSegment> segments;
    RouteSegmentsFrom(segs, m_path, {}, {}, segments);
    FillSegmentInfo(vector<double>(m_path.size() - 1, 0.0), segments);

    m_route.SetRoutingSettings(GetRoutingSettings(VehicleType::Car));
    m_route.SetRouteSegments(std::move(segments));
    m_route.SetGeometry(m_path.begin(), m_path.end());

    auto const wa = [](m2::PointD const & p)
    { return geometry::PointWithAltitude(p, geometry::kDefaultAltitudeMeters); };
    m_route.SetSubroutes(
        vector<Route::SubrouteAttrs>{Route::SubrouteAttrs(wa(m_path.front()), wa(m_b1), 0, m_stop1Idx),
                                     Route::SubrouteAttrs(wa(m_b1), wa(m_b2), m_stop1Idx, m_stop2Idx),
                                     Route::SubrouteAttrs(wa(m_b2), wa(m_path.back()), m_stop2Idx, m_path.size() - 1)});
  }

  m2::PointD m_a1, m_b1, m_a2, m_b2;
  vector<m2::PointD> m_path;
  size_t m_stop1Idx = 0, m_stop2Idx = 0;
  Route m_route;
};

// Mirrors RoutingSession::PassCheckpoints() for a route with |subrouteCount| subroutes.
size_t PassIntermediateStops(Route & route, size_t subrouteCount)
{
  size_t passed = 0;
  while (passed + 1 < subrouteCount && route.IsSubroutePassed(passed))
  {
    route.PassNextSubroute();
    ++passed;
  }
  return passed;
}

// Two consecutive off-road stops are passed in order, each shortly after its own projection root.
UNIT_TEST(PassCheckpoint_TwoStopsInOrder)
{
  TwoStopFixture f;
  auto & route = f.m_route;

  double fire1M = -1.0, fire2M = -1.0;
  size_t passedLegs = 0;
  for (double x = 0.003; x < 0.0085; x += 0.00005)
  {
    if (!route.MoveIterator(GetGps(x, 0.0)))
      continue;
    // Mirror RoutingSession::PassCheckpoints().
    while (passedLegs < 2 && route.IsSubroutePassed(passedLegs))
    {
      route.PassNextSubroute();
      ++passedLegs;
      (passedLegs == 1 ? fire1M : fire2M) = (x - (passedLegs == 1 ? f.m_a1.x : f.m_a2.x)) * kDegToM;
    }
  }

  TEST_EQUAL(passedLegs, 2, ("Both stops must be passed"));
  TEST_GREATER_OR_EQUAL(fire1M, 14.9, ());
  TEST_LESS_OR_EQUAL(fire1M, 25.0, ());
  TEST_GREATER_OR_EQUAL(fire2M, 14.9, ());
  TEST_LESS_OR_EQUAL(fire2M, 25.0, ());
}

// Ruler routes (see the layout table in ruler_router.cpp) have subroutes shorter than 3 segments,
// so they keep today's rule bit-for-bit despite consisting of fake segments only.
UNIT_TEST(PassCheckpoint_RulerLayoutUnchanged)
{
  vector<m2::PointD> const pts = {{0.0, 0.0}, {0.001, 0.0}, {0.002, 0.0}, {0.003, 0.0}};  // start, 1, 2, finish
  vector<m2::PointD> geometry;
  for (auto const & p : pts)
  {
    geometry.push_back(p);
    geometry.push_back(p);
  }

  vector<RouteSegment> routeSegments;
  vector<Segment> const segs(geometry.size() - 1, Segment(kFakeNumMwmId, 0, 0, false));
  RouteSegmentsFrom(segs, geometry, {}, {}, routeSegments);
  FillSegmentInfo(vector<double>(geometry.size() - 1, 0.0), routeSegments);

  Route route;
  route.SetRoutingSettings(GetRoutingSettings(VehicleType::Pedestrian));
  route.SetRouteSegments(std::move(routeSegments));
  route.SetGeometry(geometry.begin(), geometry.end());

  auto const wa = [](m2::PointD const & p) { return geometry::PointWithAltitude(p, geometry::kDefaultAltitudeMeters); };
  vector<Route::SubrouteAttrs> subroutes;
  for (size_t i = 1; i < pts.size(); ++i)
  {
    subroutes.emplace_back(wa(pts[i - 1]), wa(pts[i]), i * 2 - 2, i * 2);
    subroutes.emplace_back(wa(pts[i - 1]), wa(pts[i]), i * 2 - 1, i * 2);
  }
  route.SetSubroutes(std::move(subroutes));

  auto const & segments = route.GetRouteSegments();
  size_t const endSegmentIdx = route.GetSubrouteAttrs(0).GetEndSegmentIdx();
  int fireOld = -1, fireNew = -1, sample = 0;
  for (double x = 0.0; x < 0.0015; x += 0.00002, ++sample)
  {
    if (!route.MoveIterator(GetGps(x, 0.0)))
      continue;
    double const remainingM =
        segments[endSegmentIdx - 1].GetDistFromBeginningMeters() - route.GetCurrentDistanceFromBeginMeters();
    if (fireOld < 0 && remainingM < 10.0)
      fireOld = sample;
    if (fireNew < 0 && route.IsSubroutePassed(0))
      fireNew = sample;
  }

  TEST_GREATER_OR_EQUAL(fireNew, 0, ("The ruler leg must be passed"));
  TEST_EQUAL(fireOld, fireNew, ("Ruler-shaped legs must keep today's rule"));
}

// A gap in the fixes across a stop (a tunnel, a backgrounded app) leaves every position unmatched,
// so nothing is ever evaluated near the stop. Re-attaching to the next subroute passes it.
UNIT_TEST(PassCheckpoint_RejoinAfterGap)
{
  for (auto const vehicle : {VehicleType::Car, VehicleType::Bicycle, VehicleType::Pedestrian, VehicleType::Transit})
  {
    // Both on-road (0, 5 m) and off-road stops are missed when no fix lands near them.
    for (double c : {0.0, 5.0, 33.0, 200.0})
    {
      Fixture f(c, vehicle);
      for (double x = 0.0035; x < kAx - 100.0 / kDegToM; x += 0.00005)
        TEST(f.m_route.MoveIterator(GetGps(x, 0.0)), ("Must follow the route before the gap", vehicle, c));

      auto const afterGap = GetGps(kAx + 200.0 / kDegToM, 0.0);
      TEST(!f.m_route.MoveIterator(afterGap), ("The fix after the gap must not match", vehicle, c));
      TEST(!f.m_route.IsSubroutePassed(0), ("Without a rejoin the stop stays pending", vehicle, c));

      TEST(f.m_route.RejoinPastCheckpoints(afterGap), ("Must rejoin the next subroute", vehicle, c));
      TEST_GREATER_OR_EQUAL(f.m_route.GetCurrentIter().m_ind, f.m_stopIdx, ("Position must move onto leg 1", vehicle));
      TEST(f.m_route.IsSubroutePassed(0), ("The stop must be passed after the rejoin", vehicle, c));
      // Following continues on the new subroute even before the session passes the checkpoints.
      TEST(f.m_route.MoveIterator(GetGps(kAx + 250.0 / kDegToM, 0.0)), ("Must match after the rejoin", vehicle, c));
    }
  }
}

// Everything that must not count as a rejoin.
UNIT_TEST(PassCheckpoint_RejoinRejects)
{
  double constexpr kOffRoadM = 33.0;
  // Follow the route up to 100 m before A, so that the positions below are all unmatched.
  auto const followToGap = [](Fixture & f)
  {
    for (double x = 0.0035; x < kAx - 100.0 / kDegToM; x += 0.00005)
      f.m_route.MoveIterator(GetGps(x, 0.0));
  };

  // A stop farther off the road than the matching threshold is the interesting case: the next
  // subroute retraces the connector, so its first |c| metres are not progress along the route.
  for (double c : {33.0, 200.0})
  {
    Fixture f(c);
    followToGap(f);
    // Still short of the stop: such a fix projects onto the next subroute exactly at the connector
    // root, because both subroutes join the checkpoint to the road at the same point.
    for (double d : {-30.0, -60.0, -150.0})
      TEST(!f.m_route.RejoinPastCheckpoints(GetGps(kAx + d / kDegToM, 0.0)), ("Approaching the stop", c, d));

    // Past the stop but not far enough to rule out a fix taken while approaching it.
    for (double d : {5.0, 20.0, 45.0})
      TEST(!f.m_route.RejoinPastCheckpoints(GetGps(kAx + d / kDegToM, 0.0)), ("Too close to the stop", c, d));

    // Wandering off the road near the stop (a parking lot, a park path).
    TEST(!f.m_route.RejoinPastCheckpoints(GetGps(kAx - 30.0 / kDegToM, 60.0 / kDegToM)), ("Off the road", c));
    TEST(!f.m_route.RejoinPastCheckpoints(GetGps(kAx + 100.0 / kDegToM, 200.0 / kDegToM)), ("Far off the road", c));
  }
  {
    // The route turns back after the stop: driving on gives no rejoin (the user really left the
    // route), and -- the reason the whole route is scanned -- a fix still approaching the stop
    // must not be read as progress along the leg that retraces the very same road.
    Fixture f(kOffRoadM, VehicleType::Car, true /* reversed */);
    followToGap(f);
    TEST(!f.m_route.RejoinPastCheckpoints(GetGps(kAx + 200.0 / kDegToM, 0.0)), ("Driving on past a reversed leg"));
    for (double d : {-30.0, -60.0, -150.0})
      TEST(!f.m_route.RejoinPastCheckpoints(GetGps(kAx + d / kDegToM, 0.0)), ("Approaching, reversed leg, d =", d));
  }
  {
    // A route without intermediate checkpoints has nothing to skip.
    Fixture f(kOffRoadM);
    auto const wa = [](m2::PointD const & p)
    { return geometry::PointWithAltitude(p, geometry::kDefaultAltitudeMeters); };
    f.m_route.SetSubroutes(vector<Route::SubrouteAttrs>{
        Route::SubrouteAttrs(wa(f.m_path.front()), wa(f.m_path.back()), 0, f.m_path.size() - 1)});
    followToGap(f);
    TEST(!f.m_route.RejoinPastCheckpoints(GetGps(0.009, 0.0)), ("No intermediate checkpoint to pass"));
  }
}

// The rejoin needs a full matching threshold of progress past the point where the route touches the
// stop, whatever the connector length -- closer than that a fix is still explainable as approaching.
UNIT_TEST(PassCheckpoint_RejoinAcceptDistance)
{
  for (auto const vehicle : {VehicleType::Car, VehicleType::Bicycle, VehicleType::Pedestrian, VehicleType::Transit})
  {
    double const thresholdM = GetRoutingSettings(vehicle).m_matchingThresholdM;
    for (double c : {0.0, 33.0, 200.0})
    {
      Fixture f(c, vehicle);
      double firstAcceptM = -1.0;
      // 0.00001 deg ~ 1.1 m sampling; the fixture is untouched until the first accepted position.
      for (double x = kAx; x < 0.0095 && firstAcceptM < 0.0; x += 0.00001)
        if (f.m_route.RejoinPastCheckpoints(GetGps(x, 0.0)))
          firstAcceptM = MetersPastA({x, 0.0});

      TEST_ALMOST_EQUAL_ABS(firstAcceptM, thresholdM, 1.2, ("Accept distance from A", vehicle, c));
    }
  }
}

// A gap spanning two stops passes both of them at once.
UNIT_TEST(PassCheckpoint_RejoinSkipsTwoStops)
{
  TwoStopFixture f;
  for (double x = 0.003; x < 0.0045; x += 0.00005)
    TEST(f.m_route.MoveIterator(GetGps(x, 0.0)), ());

  auto const afterGap = GetGps(0.0085, 0.0);  // 167 m past the second stop.
  TEST(!f.m_route.MoveIterator(afterGap), ("The fix after the gap must not match"));
  TEST(f.m_route.RejoinPastCheckpoints(afterGap), ("Must rejoin the last subroute"));
  TEST_EQUAL(PassIntermediateStops(f.m_route, 3), 2, ("Both stops must be passed"));
}

// A route that runs over the same road twice must pass as few checkpoints as it can explain.
UNIT_TEST(PassCheckpoint_RejoinEarliestSubrouteWins)
{
  double constexpr c = 33.0;
  m2::PointD const a1(0.005, 0.0), b1(0.005, c / kDegToM);
  m2::PointD const a2(0.005, 0.0), b2(0.005, -c / kDegToM);

  // Leg 0: start -> A1 -> B1. Leg 1: B1 -> A1 -> east 0.009 -> back to A2 -> B2 (an out-and-back).
  // Leg 2: B2 -> A2 -> east 0.009 -> 0.013, i.e. it retraces leg 1's eastward half.
  vector<m2::PointD> path = {{0.0, 0.0}, {0.0, 0.0}, {0.002, 0.0}, {0.004, 0.0}, a1, b1};
  size_t const stop1Idx = path.size();
  path.push_back(b1);
  path.push_back(b1);
  path.insert(path.end(), {a1, {0.007, 0.0}, {0.009, 0.0}, {0.007, 0.0}, a2, b2});
  size_t const stop2Idx = path.size();
  path.push_back(b2);
  path.push_back(b2);
  path.insert(path.end(), {a2, {0.007, 0.0}, {0.009, 0.0}, {0.011, 0.0}, {0.013, 0.0}, {0.013, 0.0}});

  vector<Segment> segs;
  for (uint32_t i = 0; i + 1 < path.size(); ++i)
    segs.push_back(RealSeg(i));
  for (size_t idx :
       {stop1Idx - 2, stop1Idx - 1, stop1Idx, stop1Idx + 1, stop2Idx - 2, stop2Idx - 1, stop2Idx, stop2Idx + 1})
    segs[idx] = FakeSeg(static_cast<uint32_t>(idx));

  vector<RouteSegment> segments;
  RouteSegmentsFrom(segs, path, {}, {}, segments);
  FillSegmentInfo(vector<double>(path.size() - 1, 0.0), segments);

  Route route;
  route.SetRoutingSettings(GetRoutingSettings(VehicleType::Car));
  route.SetRouteSegments(std::move(segments));
  route.SetGeometry(path.begin(), path.end());

  auto const wa = [](m2::PointD const & p) { return geometry::PointWithAltitude(p, geometry::kDefaultAltitudeMeters); };
  route.SetSubroutes(
      vector<Route::SubrouteAttrs>{Route::SubrouteAttrs(wa(path.front()), wa(b1), 0, stop1Idx),
                                   Route::SubrouteAttrs(wa(b1), wa(b2), stop1Idx, stop2Idx),
                                   Route::SubrouteAttrs(wa(b2), wa(path.back()), stop2Idx, path.size() - 1)});

  for (double x = 0.003; x < 0.0045; x += 0.00005)
    TEST(route.MoveIterator(GetGps(x, 0.0)), ());

  auto const afterGap = GetGps(0.008, 0.0);  // On the road both legs 1 and 2 run over.
  TEST(!route.MoveIterator(afterGap), ());
  TEST(route.RejoinPastCheckpoints(afterGap), ());
  TEST_EQUAL(PassIntermediateStops(route, 3), 1, ("Only the first stop can be explained as passed"));
}

// Returns a prebuilt off-road-stop route, so that the session exercises the real build, promotion
// and following pipeline.
class StopRouter : public IRouter
{
public:
  explicit StopRouter(Route const & route) : m_route(route) {}

  string GetName() const override { return "stop-router"; }
  void ClearState() override {}
  void SetGuides(GuidesTracks &&) override {}

  RouterResultCode CalculateRoute(Checkpoints const &, m2::PointD const &, bool, RouterDelegate const &,
                                  RoutesResult & result) override
  {
    result.MakeFrom(GetName(), Route(m_route));
    return RouterResultCode::NoError;
  }

  bool FindClosestProjectionToRoad(m2::PointD const &, m2::PointD const &, double, EdgeProj &) override
  {
    return false;
  }

private:
  Route m_route;
};

class TimedSignal
{
public:
  void Signal()
  {
    lock_guard<mutex> guard(m_mutex);
    m_flag = true;
    m_cv.notify_one();
  }

  bool WaitUntil(chrono::steady_clock::time_point const & time)
  {
    unique_lock<mutex> lock(m_mutex);
    m_cv.wait_until(lock, time, [this, &time] { return m_flag || chrono::steady_clock::now() > time; });
    return m_flag;
  }

private:
  mutex m_mutex;
  condition_variable m_cv;
  bool m_flag = false;
};

// End to end: driving past an off-road stop passes the checkpoint and never asks for a rebuild.
UNIT_CLASS_TEST(AsyncGuiThreadTestWithRoutingSession, PassOffRoadCheckpointNoRebuild)
{
  Fixture f(33.0, VehicleType::Car);

  TimedSignal buildSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&buildSignal, &f, this]()
  {
    InitRoutingSession();
    m_session->SetRoutingSettings(GetRoutingSettings(VehicleType::Car));
    m_session->SetRouter(make_unique<StopRouter>(f.m_route), nullptr);
    m_session->SetRoutingCallbacks([&buildSignal](RoutesResult const &, RouterResultCode) { buildSignal.Signal(); },
                                   nullptr, nullptr, nullptr);
    // BuildRoute() is what gives the session real three-point checkpoints.
    m_session->BuildRoute(Checkpoints(CheckpointsGeometry{f.m_path.front(), f.m_b, f.m_path.back()}),
                          RouterDelegate::kNoTimeout);
  });
  TEST(buildSignal.WaitUntil(chrono::steady_clock::now() + chrono::seconds(30)), ("Route was not built."));

  TimedSignal driveSignal;
  bool sawRebuild = false;
  size_t passedIdx = 0;
  GetPlatform().RunTask(Platform::Thread::Gui, [&, this]()
  {
    m_session->SetCheckpointCallback([&passedIdx](size_t idx) { passedIdx = idx; });
    for (double x = 0.0035; x < 0.0075; x += 0.00005)
      sawRebuild = sawRebuild || m_session->OnLocationPositionChanged(GetGps(x, 0.0)) == SessionState::RouteNeedRebuild;
    driveSignal.Signal();
  });
  TEST(driveSignal.WaitUntil(chrono::steady_clock::now() + chrono::seconds(30)), ("Drive timeout."));

  TEST(!sawRebuild, ("Passing an off-road stop must never request a rebuild"));
  TEST_EQUAL(passedIdx, 1, ("The intermediate checkpoint must be passed"));
}

// End to end: a gap in the fixes across the stop must not send the user back to it.
UNIT_CLASS_TEST(AsyncGuiThreadTestWithRoutingSession, RejoinPastCheckpointAfterGap)
{
  Fixture f(33.0, VehicleType::Car);

  TimedSignal buildSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&buildSignal, &f, this]()
  {
    InitRoutingSession();
    m_session->SetRoutingSettings(GetRoutingSettings(VehicleType::Car));
    m_session->SetRouter(make_unique<StopRouter>(f.m_route), nullptr);
    m_session->SetRoutingCallbacks([&buildSignal](RoutesResult const &, RouterResultCode) { buildSignal.Signal(); },
                                   nullptr, nullptr, nullptr);
    m_session->BuildRoute(Checkpoints(CheckpointsGeometry{f.m_path.front(), f.m_b, f.m_path.back()}),
                          RouterDelegate::kNoTimeout);
  });
  TEST(buildSignal.WaitUntil(chrono::steady_clock::now() + chrono::seconds(30)), ("Route was not built."));

  TimedSignal driveSignal;
  bool sawRebuild = false;
  size_t passedIdx = 0;
  SessionState state = SessionState::NoValidRoute;
  GetPlatform().RunTask(Platform::Thread::Gui, [&, this]()
  {
    m_session->SetCheckpointCallback([&passedIdx](size_t idx) { passedIdx = idx; });
    // Fixes stop 100 m before the stop and resume 100 m past it.
    for (double x = 0.0035; x < kAx - 100.0 / kDegToM; x += 0.00005)
      m_session->OnLocationPositionChanged(GetGps(x, 0.0));
    for (double x = kAx + 100.0 / kDegToM; x < 0.0075; x += 0.00005)
    {
      state = m_session->OnLocationPositionChanged(GetGps(x, 0.0));
      sawRebuild = sawRebuild || state == SessionState::RouteNeedRebuild;
    }
    driveSignal.Signal();
  });
  TEST(driveSignal.WaitUntil(chrono::steady_clock::now() + chrono::seconds(30)), ("Drive timeout."));

  TEST(!sawRebuild, ("A stop passed during a gap must not trigger a rebuild back to it"));
  TEST_EQUAL(passedIdx, 1, ("The intermediate checkpoint must be passed"));
  TEST_EQUAL(state, SessionState::OnRoute, ("Following must continue"));
}

// A user who really leaves the route still gets a rebuild.
UNIT_CLASS_TEST(AsyncGuiThreadTestWithRoutingSession, RejoinFailureStillRebuilds)
{
  Fixture f(33.0, VehicleType::Car);

  TimedSignal buildSignal;
  GetPlatform().RunTask(Platform::Thread::Gui, [&buildSignal, &f, this]()
  {
    InitRoutingSession();
    m_session->SetRoutingSettings(GetRoutingSettings(VehicleType::Car));
    m_session->SetRouter(make_unique<StopRouter>(f.m_route), nullptr);
    m_session->SetRoutingCallbacks([&buildSignal](RoutesResult const &, RouterResultCode) { buildSignal.Signal(); },
                                   nullptr, nullptr, nullptr);
    m_session->BuildRoute(Checkpoints(CheckpointsGeometry{f.m_path.front(), f.m_b, f.m_path.back()}),
                          RouterDelegate::kNoTimeout);
  });
  TEST(buildSignal.WaitUntil(chrono::steady_clock::now() + chrono::seconds(30)), ("Route was not built."));

  TimedSignal driveSignal;
  bool sawRebuild = false;
  size_t passedIdx = 0;
  GetPlatform().RunTask(Platform::Thread::Gui, [&, this]()
  {
    m_session->SetCheckpointCallback([&passedIdx](size_t idx) { passedIdx = idx; });
    for (double x = 0.0035; x < kAx - 100.0 / kDegToM; x += 0.00005)
      m_session->OnLocationPositionChanged(GetGps(x, 0.0));
    // Turn off the route and drive away from it.
    for (double y = 100.0; y < 1000.0; y += 50.0)
      sawRebuild = sawRebuild || m_session->OnLocationPositionChanged(GetGps(kAx - 100.0 / kDegToM, y / kDegToM)) ==
                                     SessionState::RouteNeedRebuild;
    driveSignal.Signal();
  });
  TEST(driveSignal.WaitUntil(chrono::steady_clock::now() + chrono::seconds(30)), ("Drive timeout."));

  TEST(sawRebuild, ("Leaving the route must still request a rebuild"));
  TEST_EQUAL(passedIdx, 0, ("No checkpoint may be passed"));
}
}  // namespace checkpoint_pass_tests
