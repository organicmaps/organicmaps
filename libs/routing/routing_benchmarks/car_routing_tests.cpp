#include "testing/testing.hpp"

#include "routing/routing_benchmarks/helpers.hpp"

#include "routing/car_directions.hpp"
#include "routing/road_graph.hpp"

#include "routing_common/car_model.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"

#include <memory>
#include <set>
#include <string>

namespace
{
std::set<std::string> const kCarMapFiles = {"Russia_Moscow"};

class CarTest : public RoutingTest
{
public:
  CarTest() : RoutingTest(routing::IRoadGraph::Mode::ObeyOnewayTag, routing::VehicleType::Car, kCarMapFiles) {}

  void TestCarRouter(ms::LatLon const & start, ms::LatLon const & final, size_t reiterations)
  {
    routing::Route routeFoundByAstarBidirectional("", 0 /* route id */);
    auto router = CreateRouter("test-astar-bidirectional");

    m2::PointD const startMerc = mercator::FromLatLon(start);
    m2::PointD const finalMerc = mercator::FromLatLon(final);
    for (size_t i = 0; i < reiterations; ++i)
      TestRouter(*router, startMerc, finalMerc, routeFoundByAstarBidirectional);
  }

protected:
  std::unique_ptr<routing::VehicleModelFactoryInterface> CreateModelFactory() override
  {
    return std::make_unique<SimplifiedModelFactory<routing::CarModel>>();
  }
};

// Benchmarks below are on profiling looking for the best segments of routes.
// So short routes are used to focus on time needed for finding the best segments.

// Start and finish are located in a city with dense road network.
UNIT_CLASS_TEST(CarTest, InCity)
{
  TestCarRouter(ms::LatLon(55.75785, 37.58267), ms::LatLon(55.76082, 37.58492), 30);
}

// Start and finish are located near a big road.
UNIT_CLASS_TEST(CarTest, BigRoad)
{
  TestCarRouter(ms::LatLon(55.75826, 37.39476), ms::LatLon(55.7605, 37.39003), 30);
}

// Start are located near an airport center. It's far from road network.
UNIT_CLASS_TEST(CarTest, InAirport)
{
  TestCarRouter(ms::LatLon(55.97285, 37.41275), ms::LatLon(55.96396, 37.41922), 30);
}
}  // namespace
