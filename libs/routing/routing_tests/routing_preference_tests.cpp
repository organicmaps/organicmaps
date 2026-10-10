#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_generator.hpp"

#include "routing/index_router.hpp"
#include "routing/route.hpp"
#include "routing/routing_options.hpp"
#include "routing/world_graph.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"

#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include "traffic/traffic_cache.hpp"

#include <functional>

namespace routing_preference_tests
{
using namespace routing;

class RoutingPreferenceFixture : public generator::tests_support::TestRawGenerator
{
public:
  FrozenDataSource m_data;
  traffic::TrafficCache m_traffic;
  std::shared_ptr<NumMwmIds> m_ids = std::make_shared<NumMwmIds>();
  std::shared_ptr<m4::Tree<NumMwmId>> m_tree = std::make_shared<m4::Tree<NumMwmId>>();
  m2::RectD m_bounds;
  uint32_t m_ferryFeature = 0;
  std::function<void()> m_onCountryLookup;

  RoutingPreferenceFixture()
  {
    BuildFB(GetPlatform().ReadPathForFile("test_data/osm/routing_options.osm"), "Germany");
    BuildFeatures("Germany");
    BuildRouting("Germany", "Germany");
    BuildCrossMwm("Germany", "Germany");
    ForEachFeature("Germany", [this](auto const & ft)
    {
      if (feature::TypesHolder(*ft).Has(classif().GetTypeByPath({"route", "ferry"})))
        m_ferryFeature = ft->GetID().m_index;
    });
    auto const file = platform::LocalCountryFile::MakeTemporary(GetMwmPath("Germany"));
    auto const [id, status] = m_data.RegisterMap(file);
    CHECK_EQUAL(status, MwmSet::RegResult::Success, ());
    m_ids->RegisterFile(file.GetCountryFile());
    auto const handle = m_data.GetMwmHandleById(id);
    m_bounds = handle.GetValue()->GetHeader().GetBounds();
    m_tree->Add(m_ids->GetId(file.GetCountryFile()), m_bounds);
  }

  std::unique_ptr<IndexRouter> Router(VehicleType vehicle)
  {
    return std::make_unique<IndexRouter>(vehicle, false, [](std::string const &) { return std::string(); },
                                         [this](m2::PointD const &)
    {
      if (m_onCountryLookup)
        m_onCountryLookup();
      return "Germany";
    }, [this](std::string const &) { return m_bounds; }, m_ids, m_tree, m_traffic, m_data);
  }

  static bool UsesFerry(RouteBase const & route)
  {
    for (auto const & segment : route.GetRouteSegments())
      if (segment.GetRoadTypes().Has(RoutingOptions::Ferry))
        return true;
    return false;
  }

  static RouterResultCode Calculate(IndexRouter & router, RoutesResult & result, RouterDelegate const & delegate = {},
                                    bool adjust = false, bool alternatives = false, bool intermediate = false)
  {
    // The finish is over 10 km away so an unchanged request can reuse the adjustment cache.
    // The cached first leg ends at a road vertex so off-road connector costs stay within the
    // five-minute adjustment budget.
    std::vector<m2::PointD> points = {intermediate
                                          ? mercator::FromLatLon(adjust ? 0.9999 : 1.0001, adjust ? 1.0103 : 1.0101)
                                          : mercator::FromLatLon(1, 1.01)};
    if (intermediate)
      points.push_back(mercator::FromLatLon(1.0001, 1.0105));
    points.push_back(mercator::FromLatLon(1, 1.49));
    return router.CalculateRoute(Checkpoints(std::move(points)), {}, adjust, alternatives, delegate, result);
  }

private:
  RoutingOptionSetter m_car{0, VehicleType::Car};
  RoutingOptionSetter m_bicycle{0, VehicleType::Bicycle};
  RoutingOptionSetter m_pedestrian{0, VehicleType::Pedestrian};
};

UNIT_CLASS_TEST(RoutingPreferenceFixture, CarFerryAvoidanceDoesNotAffectOtherRoadModes)
{
  RoutingOptions::SaveToSettings(VehicleType::Car, RoutingOptions(RoutingOptions::Ferry));
  for (auto const vehicle : {VehicleType::Car, VehicleType::Bicycle, VehicleType::Pedestrian})
  {
    auto router = Router(vehicle);
    RoutesResult result;
    TEST_EQUAL(Calculate(*router, result), RouterResultCode::NoError, (vehicle));
    TEST_EQUAL(UsesFerry(result.GetActive()), vehicle != VehicleType::Car, (vehicle));
  }
}

UNIT_CLASS_TEST(RoutingPreferenceFixture, FerryAvoidanceIsIndependent)
{
  for (auto const avoided : {VehicleType::Bicycle, VehicleType::Pedestrian})
  {
    RoutingOptions::SaveToSettings(VehicleType::Bicycle, {});
    RoutingOptions::SaveToSettings(VehicleType::Pedestrian, {});
    RoutingOptions::SaveToSettings(avoided, RoutingOptions(RoutingOptions::Ferry));
    for (auto const vehicle : {VehicleType::Car, VehicleType::Bicycle, VehicleType::Pedestrian})
    {
      auto router = Router(vehicle);
      RoutesResult result;
      TEST_EQUAL(Calculate(*router, result), RouterResultCode::NoError, (vehicle));
      TEST_EQUAL(UsesFerry(result.GetActive()), vehicle != avoided, (vehicle, avoided));
    }
  }
}

UNIT_CLASS_TEST(RoutingPreferenceFixture, ChangedAvoidanceInvalidatesCachedSuffix)
{
  auto router = Router(VehicleType::Car);
  RoutesResult before;
  TEST_EQUAL(Calculate(*router, before, {}, false, true, true), RouterResultCode::NoError, ());
  TEST(UsesFerry(before.GetActive()), ());
  RoutesResult unchanged;
  TEST_EQUAL(Calculate(*router, unchanged, {}, true, false, true), RouterResultCode::NoError, ());
  TEST(UsesFerry(unchanged.GetActive()), ());
  RoutingOptions::SaveToSettings(VehicleType::Car, RoutingOptions(RoutingOptions::Ferry));
  RoutesResult after;
  TEST_EQUAL(Calculate(*router, after, {}, true, true, true), RouterResultCode::NoError, ());
  for (auto const & route : after.m_routes)
    TEST(!UsesFerry(route), ());
}

UNIT_CLASS_TEST(RoutingPreferenceFixture, RecreatedGraphsUseTheRequestSnapshot)
{
  auto router = Router(VehicleType::Car);
  bool changed = false;
  m_onCountryLookup = [&]
  {
    if (changed)
      return;
    changed = true;
    RoutingOptions::SaveToSettings(VehicleType::Car, RoutingOptions(RoutingOptions::Ferry));
    auto graph = router->MakeSingleMwmWorldGraph();
    TEST(graph->IsRoutingOptionsGood(Segment(0, m_ferryFeature, 0, true)), ());
  };
  RoutesResult result;
  TEST_EQUAL(Calculate(*router, result, {}, false, true), RouterResultCode::NoError, ());
  TEST(changed, ());
  TEST(UsesFerry(result.GetActive()), ());
  auto graph = router->MakeSingleMwmWorldGraph();
  TEST(!graph->IsRoutingOptionsGood(Segment(0, m_ferryFeature, 0, true)), ());
}
}  // namespace routing_preference_tests
