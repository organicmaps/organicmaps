#include "testing/testing.hpp"

#include "routing/routing_benchmarks/helpers.hpp"

#include "routing/car_directions.hpp"
#include "routing/road_graph.hpp"

#include "routing_common/bicycle_model.hpp"

#include "geometry/mercator.hpp"

#include <memory>
#include <set>
#include <string>

namespace
{
// Test preconditions: files from the kBicycleMapFiles set with '.mwm'
// extension must be placed in omim/data folder.
std::set<std::string> const kBicycleMapFiles = {"Russia_Moscow"};

class BicycleTest : public RoutingTest
{
public:
  BicycleTest() : RoutingTest(routing::IRoadGraph::Mode::ObeyOnewayTag, routing::VehicleType::Bicycle, kBicycleMapFiles)
  {}

protected:
  std::unique_ptr<routing::VehicleModelFactoryInterface> CreateModelFactory() override
  {
    return std::make_unique<SimplifiedModelFactory<routing::BicycleModel>>();
  }
};

UNIT_CLASS_TEST(BicycleTest, Smoke)
{
  m2::PointD const start = mercator::FromLatLon(55.79181, 37.56513);
  m2::PointD const final = mercator::FromLatLon(55.79369, 37.56054);
  TestRouters(start, final);
}

UNIT_CLASS_TEST(BicycleTest, RussiaMoscow_Test1)
{
  m2::PointD const start = mercator::FromLatLon(55.79828, 37.53710);
  m2::PointD const final = mercator::FromLatLon(55.79956, 37.54115);
  TestRouters(start, final);
}
}  // namespace
