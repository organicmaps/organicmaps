#include "testing/testing.hpp"

#include "indexer/index.hpp"
#include "indexer/classificator_loader.hpp"

#include "routing/features_road_graph.hpp"
#include "routing/road_graph_router.hpp"
#include "routing/route.hpp"
#include "routing/pedestrian_model.hpp"
#include "routing/router_delegate.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/timer.hpp"

#include "std/string.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

using platform::CountryFile;
using platform::LocalCountryFile;

namespace
{

string const MAP_NAME = "UK_England";

// Since for test purposes we compare routes lengths to check algorithms consistency,
// we should use simplified pedestrian model, where all available edges have max speed
class SimplifiedPedestrianModel : public routing::PedestrianModel
{
public:
  // IVehicleModel override:
  // PedestrianModel::GetSpeed filters features and returns zero speed if feature
  // is not allowed for pedestrians or otherwise some speed depending of road type (0 <= speed <= maxSpeed).
  // For tests purposes for all allowed features speed must be the same as maxSpeed.
  virtual double GetSpeed(FeatureType const & f) const override
  {
    double const speed = routing::PedestrianModel::GetSpeed(f);
    if (speed <= 0.0)
      return 0.0;
    return routing::PedestrianModel::GetMaxSpeed();
  }
};

class SimplifiedPedestrianModelFactory : public routing::IVehicleModelFactory
{
public:
  SimplifiedPedestrianModelFactory()
    : m_model(make_shared<SimplifiedPedestrianModel>())
  {}

  // IVehicleModelFactory overrides:
  shared_ptr<routing::IVehicleModel> GetVehicleModel() const override { return m_model; }
  shared_ptr<routing::IVehicleModel> GetVehicleModelForCountry(string const & /*country*/) const override { return m_model; }

private:
  shared_ptr<routing::IVehicleModel> const m_model;
};

unique_ptr<routing::IRouter> CreatePedestrianAStarTestRouter(Index & index)
{
  auto UKGetter = [](m2::PointD const & /* point */){return "UK_England";};
  unique_ptr<routing::IVehicleModelFactory> vehicleModelFactory(new SimplifiedPedestrianModelFactory());
  unique_ptr<routing::IRoutingAlgorithm> algorithm(new routing::AStarRoutingAlgorithm());
  unique_ptr<routing::IRouter> router(new routing::RoadGraphRouter("test-astar-pedestrian", index, UKGetter, move(vehicleModelFactory), move(algorithm), nullptr));
  return router;
}

unique_ptr<routing::IRouter> CreatePedestrianAStarBidirectionalTestRouter(Index & index)
{
  auto UKGetter = [](m2::PointD const & /* point */){return "UK_England";};
  unique_ptr<routing::IVehicleModelFactory> vehicleModelFactory(new SimplifiedPedestrianModelFactory());
  unique_ptr<routing::IRoutingAlgorithm> algorithm(new routing::AStarBidirectionalRoutingAlgorithm());
  unique_ptr<routing::IRouter> router(new routing::RoadGraphRouter("test-astar-bidirectional-pedestrian", index, UKGetter, move(vehicleModelFactory), move(algorithm), nullptr));
  return router;
}

m2::PointD GetPointOnEdge(routing::Edge & e, double posAlong)
{
  if (posAlong <= 0.0)
    return e.GetStartJunction().GetPoint();
  if (posAlong >= 1.0)
    return e.GetEndJunction().GetPoint();
  m2::PointD const d = e.GetEndJunction().GetPoint() - e.GetStartJunction().GetPoint();
  return e.GetStartJunction().GetPoint() + d * posAlong;
}

void GetNearestPedestrianEdges(Index & index, m2::PointD const & pt, vector<pair<routing::Edge, m2::PointD>> & edges)
{
  unique_ptr<routing::IVehicleModelFactory> vehicleModelFactory(new SimplifiedPedestrianModelFactory());
  routing::FeaturesRoadGraph roadGraph(index, move(vehicleModelFactory));

  roadGraph.FindClosestEdges(pt, 1 /*count*/, edges);
}

void TestRouter(routing::IRouter & router, m2::PointD const & startPos, m2::PointD const & finalPos, routing::Route & foundRoute)
{
  routing::RouterDelegate delegate;
  LOG(LINFO, ("Calculating routing ...", router.GetName()));
  routing::Route route("");
  my::Timer timer;
  routing::IRouter::ResultCode const resultCode = router.CalculateRoute(
      startPos, m2::PointD::Zero() /* startDirection */, finalPos, delegate, route);
  double const elapsedSec = timer.ElapsedSeconds();
  TEST_EQUAL(routing::IRouter::NoError, resultCode, ());
  LOG(LINFO, ("Route polyline size:", route.GetPoly().GetSize()));
  LOG(LINFO, ("Route distance, meters:", route.GetTotalDistanceMeters()));
  LOG(LINFO, ("Elapsed, seconds:", elapsedSec));
  foundRoute.Swap(route);
}

void TestRouters(Index & index, m2::PointD const & startPos, m2::PointD const & finalPos)
{
  // find route by A*-bidirectional algorithm
  routing::Route routeFoundByAstarBidirectional("");
  unique_ptr<routing::IRouter> router = CreatePedestrianAStarBidirectionalTestRouter(index);
  TestRouter(*router, startPos, finalPos, routeFoundByAstarBidirectional);

  // find route by A* algorithm
  routing::Route routeFoundByAstar("");
  router = CreatePedestrianAStarTestRouter(index);

  TestRouter(*router, startPos, finalPos, routeFoundByAstar);

  double constexpr kEpsilon = 1e-6;
  TEST(my::AlmostEqualAbs(routeFoundByAstar.GetTotalDistanceMeters(),
                          routeFoundByAstarBidirectional.GetTotalDistanceMeters(), kEpsilon), ());
}

void TestTwoPointsOnFeature(m2::PointD const & startPos, m2::PointD const & finalPos)
{
  classificator::Load();

  CountryFile countryFile(MAP_NAME);
  LocalCountryFile localFile = LocalCountryFile::MakeForTesting(MAP_NAME);

  Index index;
  UNUSED_VALUE(index.RegisterMap(localFile));
  TEST(index.IsLoaded(countryFile), ());
  MwmSet::MwmId const id = index.GetMwmIdByCountryFile(countryFile);
  TEST(id.IsAlive(), ());

  vector<pair<routing::Edge, m2::PointD>> startEdges;
  GetNearestPedestrianEdges(index, startPos, startEdges);
  TEST(!startEdges.empty(), ());

  vector<pair<routing::Edge, m2::PointD>> finalEdges;
  GetNearestPedestrianEdges(index, finalPos, finalEdges);
  TEST(!finalEdges.empty(), ());

  m2::PointD const startPosOnFeature =
      GetPointOnEdge(startEdges.front().first, 0.0 /* the start point of the feature */);
  m2::PointD const finalPosOnFeature =
      GetPointOnEdge(finalEdges.front().first, 1.0 /* the end point of the feature */);

  TestRouters(index, startPosOnFeature, finalPosOnFeature);
}

void TestTwoPoints(m2::PointD const & startPos, m2::PointD const & finalPos)
{
  classificator::Load();

  CountryFile countryFile(MAP_NAME);
  LocalCountryFile localFile = LocalCountryFile::MakeForTesting(MAP_NAME);

  Index index;
  UNUSED_VALUE(index.RegisterMap(localFile));
  TEST(index.IsLoaded(countryFile), ());
  MwmSet::MwmId const id = index.GetMwmIdByCountryFile(countryFile);
  TEST(id.IsAlive(), ());

  TestRouters(index, startPos, finalPos);
}

}  // namespace

// Tests on features

UNIT_TEST(PedestrianRouting_UK_Long1)
{
  TestTwoPointsOnFeature(m2::PointD(-1.88798, 61.90292),
                         m2::PointD(-2.06025, 61.82824));
}

UNIT_TEST(PedestrianRouting_UK_Long2)
{
  TestTwoPointsOnFeature(m2::PointD(-0.20434, 60.27445),
                         m2::PointD( 0.06962, 60.33909));
}

UNIT_TEST(PedestrianRouting_UK_Long3)
{
  TestTwoPointsOnFeature(m2::PointD(-0.07706, 60.42876),
                         m2::PointD(-0.11058, 60.20991));
}

UNIT_TEST(PedestrianRouting_UK_Long4)
{
  TestTwoPointsOnFeature(m2::PointD(-0.48574, 60.05082),
                         m2::PointD(-0.45973, 60.56715));
}

UNIT_TEST(PedestrianRouting_UK_Long5)
{
  TestTwoPointsOnFeature(m2::PointD(0.11646, 60.57330),
                         m2::PointD(0.05767, 59.93019));
}

UNIT_TEST(PedestrianRouting_UK_Long6)
{
  TestTwoPointsOnFeature(m2::PointD(0.02771, 60.49348),
                         m2::PointD(0.06533, 59.93155));
}

UNIT_TEST(PedestrianRouting_UK_Medium1)
{
  TestTwoPointsOnFeature(m2::PointD(-0.10461, 60.29721),
                         m2::PointD(-0.07532, 60.35180));
}

UNIT_TEST(PedestrianRouting_UK_Medium2)
{
  TestTwoPointsOnFeature(m2::PointD(-0.17925, 60.06331),
                         m2::PointD(-0.09959, 60.06880));
}

UNIT_TEST(PedestrianRouting_UK_Medium3)
{
  TestTwoPointsOnFeature(m2::PointD(-0.26440, 60.16831),
                         m2::PointD(-0.20113, 60.20884));
}

UNIT_TEST(PedestrianRouting_UK_Medium4)
{
  TestTwoPointsOnFeature(m2::PointD(-0.25296, 60.46539),
                         m2::PointD(-0.10975, 60.43955));
}

UNIT_TEST(PedestrianRouting_UK_Medium5)
{
  TestTwoPointsOnFeature(m2::PointD(-0.03115, 60.31819),
                         m2::PointD( 0.07400, 60.33662));
}

UNIT_TEST(PedestrianRouting_UK_Short1)
{
  TestTwoPointsOnFeature(m2::PointD(-0.10461, 60.29721),
                         m2::PointD(-0.11905, 60.29747));
}

UNIT_TEST(PedestrianRouting_UK_Short2)
{
  TestTwoPointsOnFeature(m2::PointD(-0.11092, 60.27172),
                         m2::PointD(-0.08159, 60.27623));
}

UNIT_TEST(PedestrianRouting_UK_Short3)
{
  TestTwoPointsOnFeature(m2::PointD(-0.09449, 60.25051),
                         m2::PointD(-0.06520, 60.26647));
}

// Tests on points

UNIT_TEST(PedestrianRouting_UK_Test1)
{
  TestTwoPoints(m2::PointD(-0.23371, 60.18821),
                m2::PointD(-0.27958, 60.25155));
}

UNIT_TEST(PedestrianRouting_UK_Test2)
{
  TestTwoPoints(m2::PointD(-0.23204, 60.22073),
                m2::PointD(-0.25325, 60.34312));
}

UNIT_TEST(PedestrianRouting_UK_Test3)
{
  TestTwoPoints(m2::PointD(-0.13493, 60.21329),
                m2::PointD(-0.07502, 60.38699));
}

UNIT_TEST(PedestrianRouting_UK_Test4)
{
  TestTwoPoints(m2::PointD(0.07362, 60.24965),
                m2::PointD(0.06262, 60.30536));
}

UNIT_TEST(PedestrianRouting_UK_Test6)
{
  TestTwoPoints(m2::PointD(0.12973, 60.28698),
                m2::PointD(0.16166, 60.32989));
}

UNIT_TEST(PedestrianRouting_UK_Test7)
{
  TestTwoPoints(m2::PointD(0.24339, 60.22193),
                m2::PointD(0.30297, 60.47235));
}

UNIT_TEST(PedestrianRouting_UK_Test9)
{
  TestTwoPoints(m2::PointD( 0.01390, 60.24852),
                m2::PointD(-0.01102, 60.29319));
}

UNIT_TEST(PedestrianRouting_UK_Test10)
{
  TestTwoPoints(m2::PointD(-1.26084, 60.68840),
                m2::PointD(-1.34027, 60.37865));
}

UNIT_TEST(PedestrianRouting_UK_Test11)
{
  TestTwoPoints(m2::PointD(-1.26084, 60.68840),
                m2::PointD(-1.34027, 60.37865));
}

UNIT_TEST(PedestrianRouting_UK_Test14)
{
  TestTwoPoints(m2::PointD(-0.49921, 60.50093),
                m2::PointD(-0.42539, 60.46021));
}

UNIT_TEST(PedestrianRouting_UK_Test15)
{
  TestTwoPoints(m2::PointD(-0.35293, 60.38324),
                m2::PointD(-0.27232, 60.48594));
}

UNIT_TEST(PedestrianRouting_UK_Test16)
{
  TestTwoPoints(m2::PointD(-0.24521, 60.41771),
                m2::PointD(0.052673, 60.48102));
}

UNIT_TEST(PedestrianRouting_UK_Test17)
{
  TestTwoPoints(m2::PointD(0.60492, 60.36565),
                m2::PointD(0.59411, 60.31529));
}

UNIT_TEST(PedestrianRouting_UK_Test19)
{
  TestTwoPoints(m2::PointD(-0.42411, 60.22511),
                m2::PointD(-0.44178, 60.37796));
}

UNIT_TEST(PedestrianRouting_UK_Test20)
{
  TestTwoPoints(m2::PointD(0.08776, 60.05433),
                m2::PointD(0.19336, 60.38398));
}

UNIT_TEST(PedestrianRouting_UK_Test21)
{
  TestTwoPoints(m2::PointD(0.23038, 60.43846),
                m2::PointD(0.18335, 60.46692));
}

UNIT_TEST(PedestrianRouting_UK_Test22)
{
  TestTwoPoints(m2::PointD(-0.33907, 60.691735),
                m2::PointD(-0.17824, 60.478512));
}

UNIT_TEST(PedestrianRouting_UK_Test23)
{
  TestTwoPoints(m2::PointD(-0.02557, 60.41371),
                m2::PointD( 0.05972, 60.31413));
}

UNIT_TEST(PedestrianRouting_UK_Test24)
{
  TestTwoPoints(m2::PointD(-0.12511, 60.23813),
                m2::PointD(-0.27656, 60.05896));
}

UNIT_TEST(PedestrianRouting_UK_Test25)
{
  TestTwoPoints(m2::PointD(-0.12511, 60.23813),
                m2::PointD(-0.27656, 60.05896));
}

UNIT_TEST(PedestrianRouting_UK_Test26)
{
  TestTwoPoints(m2::PointD(-3.04538, 63.44428),
                m2::PointD(-2.98887, 63.47582));
}

UNIT_TEST(PedestrianRouting_UK_Test27)
{
  TestTwoPoints(m2::PointD(-2.94653, 63.61187),
                m2::PointD(-2.83215, 63.51525));
}

UNIT_TEST(PedestrianRouting_UK_Test28)
{
  TestTwoPoints(m2::PointD(-2.85275, 63.42478),
                m2::PointD(-2.88245, 63.38932));
}

UNIT_TEST(PedestrianRouting_UK_Test29)
{
  TestTwoPoints(m2::PointD(-2.35266, 63.59979),
                m2::PointD(-2.29857, 63.54677));
}

UNIT_TEST(PedestrianRouting_UK_Test30)
{
  TestTwoPoints(m2::PointD(-2.22043, 63.41066),
                m2::PointD(-2.29619, 63.65305));
}

UNIT_TEST(PedestrianRouting_UK_Test31)
{
  TestTwoPoints(m2::PointD(-2.28078, 63.66735),
                m2::PointD(-2.25378, 63.62744));
}

// This is very slow pedestrian tests (more than 20 minutes).
#ifdef SLOW_TESTS
UNIT_TEST(PedestrianRouting_UK_Test5)
{
  TestTwoPoints(m2::PointD(0.07362, 60.24965),
                m2::PointD(0.06262, 60.30536));
}

UNIT_TEST(PedestrianRouting_UK_Test8)
{
  TestTwoPoints(m2::PointD(-0.09007, 59.93887),
                m2::PointD(-0.36591, 60.38306));
}

UNIT_TEST(PedestrianRouting_UK_Test12)
{
  TestTwoPoints(m2::PointD(-0.41581, 60.05507),
                m2::PointD(-0.00499, 60.55921));
}

UNIT_TEST(PedestrianRouting_UK_Test13)
{
  TestTwoPoints(m2::PointD(-0.00847, 60.17501),
                m2::PointD(-0.38291, 60.48435));
}

UNIT_TEST(PedestrianRouting_UK_Test18)
{
  TestTwoPoints(m2::PointD( 0.57712, 60.31156),
                m2::PointD(-1.09911, 59.24341));
}
#endif
