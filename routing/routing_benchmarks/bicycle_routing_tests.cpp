#include "testing/testing.hpp"

#include "routing/routing_benchmarks/helpers.hpp"

#include "routing/bicycle_directions.hpp"
#include "routing/road_graph.hpp"

#include "routing_common/bicycle_model.hpp"

#include "geometry/mercator.hpp"

#include "std/set.hpp"
#include "std/string.hpp"

namespace
{
// Test preconditions: files from the kMapFiles set with '.mwm'
// extension must be placed in omim/data folder.
set<string> const kMapFiles = {"Russia_Moscow"};

class BicycleTest : public RoutingTest
{
public:
  BicycleTest() : RoutingTest(routing::IRoadGraph::Mode::ObeyOnewayTag, kMapFiles) {}

protected:
  // RoutingTest overrides:
  unique_ptr<routing::IDirectionsEngine> CreateDirectionsEngine(
    shared_ptr<routing::NumMwmIds> numMwmIds) override
  {
    unique_ptr<routing::IDirectionsEngine> engine(new routing::BicycleDirectionsEngine(
        m_index, numMwmIds));
    return engine;
  }

  unique_ptr<routing::VehicleModelFactoryInterface> CreateModelFactory() override
  {
    unique_ptr<routing::VehicleModelFactoryInterface> factory(
        new SimplifiedModelFactory<routing::BicycleModel>());
    return factory;
  }
};

UNIT_CLASS_TEST(BicycleTest, Smoke)
{
  m2::PointD const start = MercatorBounds::FromLatLon(55.79181, 37.56513);
  m2::PointD const final = MercatorBounds::FromLatLon(55.79369, 37.56054);
  TestRouters(start, final);
}

UNIT_CLASS_TEST(BicycleTest, RussiaMoscow_Test1)
{
  m2::PointD const start = MercatorBounds::FromLatLon(55.79828, 37.53710);
  m2::PointD const final = MercatorBounds::FromLatLon(55.79956, 37.54115);
  TestRouters(start, final);
}
}  // namespace
