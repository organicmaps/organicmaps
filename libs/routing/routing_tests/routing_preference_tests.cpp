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
#include <map>

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

  explicit RoutingPreferenceFixture(std::string const & fixture = "routing_options.osm")
  {
    BuildFB(GetPlatform().ReadPathForFile("test_data/osm/" + fixture), "Germany");
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

  static bool UsesRoadType(RouteBase const & route, RoutingOptions::Road road)
  {
    for (auto const & segment : route.GetRouteSegments())
      if (segment.GetRoadTypes().Has(road))
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

class SurfaceRoutingFixture : public RoutingPreferenceFixture
{
public:
  SurfaceRoutingFixture() : RoutingPreferenceFixture("routing_surfaces.osm")
  {
    ForEachFeature("Germany", [this](auto const & feature)
    { m_features.emplace(feature->GetDefaultName(), feature->GetID().m_index); });
  }

  Segment NamedSegment(std::string const & name) const { return Segment(0, m_features.at(name), 0, true); }

  static RouterResultCode CalculateRequiredRoad(IndexRouter & router, RoutesResult & result)
  {
    return router.CalculateRoute(Checkpoints(mercator::FromLatLon(2, 1.01), mercator::FromLatLon(2, 1.09)), {}, false,
                                 true, {}, result);
  }

private:
  std::map<std::string, uint32_t> m_features;
};

UNIT_CLASS_TEST(SurfaceRoutingFixture, BicycleDirtyMatchesMappedUnpavedSurfaces)
{
  RoutingOptions::SaveToSettings(VehicleType::Bicycle, RoutingOptions(RoutingOptions::Dirty));
  auto router = Router(VehicleType::Bicycle);
  auto graph = router->MakeSingleMwmWorldGraph();
  std::pair<char const *, bool> const cases[] = {{"Paved track", false},          {"Unknown track", false},
                                                 {"Generic road", false},         {"Paved road", false},
                                                 {"Unpaved cycleway", true},      {"Bad unpaved cycleway", true},
                                                 {"Unpaved road", true},          {"Smoothness track", true},
                                                 {"Tracktype-only track", false}, {"Trail without surface", false},
                                                 {"Compacted cycleway", true},    {"Rough paved track", false}};
  for (auto const & [name, unpaved] : cases)
  {
    auto const segment = NamedSegment(name);
    TEST_EQUAL(graph->GetRoutingOptions(segment).Has(RoutingOptions::Dirty), unpaved, (name));
    TEST_EQUAL(graph->IsRoutingOptionsGood(segment), !unpaved, (name));
  }
}

UNIT_CLASS_TEST(SurfaceRoutingFixture, CarDirtyMatchesRoadClassesAndSurfaces)
{
  RoutingOptions::SaveToSettings(VehicleType::Car, RoutingOptions(RoutingOptions::Dirty));
  auto router = Router(VehicleType::Car);
  auto graph = router->MakeSingleMwmWorldGraph();
  std::pair<char const *, bool> const cases[] = {
      {"Paved track", true},  {"Unknown track", true},    {"Generic road", true},         {"Paved road", false},
      {"Unpaved road", true}, {"Smoothness track", true}, {"Tracktype-only track", true}, {"Rough paved track", true}};
  for (auto const & [name, dirty] : cases)
  {
    auto const segment = NamedSegment(name);
    TEST_EQUAL(graph->GetRoutingOptions(segment).Has(RoutingOptions::Dirty), dirty, (name));
    TEST_EQUAL(graph->IsRoutingOptionsGood(segment), !dirty, (name));
  }
}

UNIT_CLASS_TEST(SurfaceRoutingFixture, BicycleUnpavedAvoidanceIsIndependentAndReversible)
{
  RoutingOptions::SaveToSettings(VehicleType::Car, RoutingOptions(RoutingOptions::Dirty));
  RoutingOptions::SaveToSettings(VehicleType::Bicycle, RoutingOptions(RoutingOptions::Dirty));
  auto bicycle = Router(VehicleType::Bicycle);
  auto pedestrian = Router(VehicleType::Pedestrian);
  RoutesResult walking;
  TEST_EQUAL(Calculate(*pedestrian, walking), RouterResultCode::NoError, ());
  TEST(UsesRoadType(walking.GetActive(), RoutingOptions::Dirty), ());
  for (bool const avoid : {false, true, false})
  {
    RoutingOptions::SaveToSettings(VehicleType::Bicycle, RoutingOptions(avoid ? RoutingOptions::Dirty : 0));
    RoutesResult result;
    TEST_EQUAL(Calculate(*bicycle, result, {}, true, true), RouterResultCode::NoError, (avoid));
    TEST_EQUAL(UsesRoadType(result.GetActive(), RoutingOptions::Dirty), !avoid, (avoid));
    if (avoid)
      for (auto const & route : result.m_routes)
        TEST(!UsesRoadType(route, RoutingOptions::Dirty), ());
  }
}

UNIT_CLASS_TEST(SurfaceRoutingFixture, CarAndBicycleUnpavedAvoidanceAreIndependent)
{
  for (auto const avoided : {VehicleType::Car, VehicleType::Bicycle})
  {
    RoutingOptions::SaveToSettings(VehicleType::Car,
                                   RoutingOptions(avoided == VehicleType::Car ? RoutingOptions::Dirty : 0));
    RoutingOptions::SaveToSettings(VehicleType::Bicycle,
                                   RoutingOptions(avoided == VehicleType::Bicycle ? RoutingOptions::Dirty : 0));
    auto router = Router(avoided == VehicleType::Car ? VehicleType::Bicycle : VehicleType::Car);
    RoutesResult result;
    TEST_EQUAL(CalculateRequiredRoad(*router, result), RouterResultCode::NoError, (avoided));
    TEST(UsesRoadType(result.GetActive(), RoutingOptions::Dirty), (avoided));
  }
}

UNIT_CLASS_TEST(SurfaceRoutingFixture, RequiredUnpavedRoadFailsBicycleRouting)
{
  auto router = Router(VehicleType::Bicycle);
  RoutesResult before;
  TEST_EQUAL(CalculateRequiredRoad(*router, before), RouterResultCode::NoError, ());
  TEST(UsesRoadType(before.GetActive(), RoutingOptions::Dirty), ());
  RoutingOptions::SaveToSettings(VehicleType::Bicycle, RoutingOptions(RoutingOptions::Dirty));
  RoutesResult after;
  // Avoidance can fail during endpoint snapping or path search.
  auto const code = CalculateRequiredRoad(*router, after);
  TEST(code == RouterResultCode::StartPointNotFound || code == RouterResultCode::EndPointNotFound ||
           code == RouterResultCode::RouteNotFound,
       (code));
  TEST(after.m_routes.empty(), ());
}

UNIT_CLASS_TEST(RoutingPreferenceFixture, CarFerryAvoidanceDoesNotAffectOtherRoadModes)
{
  RoutingOptions::SaveToSettings(VehicleType::Car, RoutingOptions(RoutingOptions::Ferry));
  for (auto const vehicle : {VehicleType::Car, VehicleType::Bicycle, VehicleType::Pedestrian})
  {
    auto router = Router(vehicle);
    RoutesResult result;
    TEST_EQUAL(Calculate(*router, result), RouterResultCode::NoError, (vehicle));
    TEST_EQUAL(UsesRoadType(result.GetActive(), RoutingOptions::Ferry), vehicle != VehicleType::Car, (vehicle));
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
      TEST_EQUAL(UsesRoadType(result.GetActive(), RoutingOptions::Ferry), vehicle != avoided, (vehicle, avoided));
    }
  }
}

UNIT_CLASS_TEST(RoutingPreferenceFixture, ChangedAvoidanceInvalidatesCachedSuffix)
{
  auto router = Router(VehicleType::Car);
  RoutesResult before;
  TEST_EQUAL(Calculate(*router, before, {}, false, true, true), RouterResultCode::NoError, ());
  TEST(UsesRoadType(before.GetActive(), RoutingOptions::Ferry), ());
  RoutesResult unchanged;
  TEST_EQUAL(Calculate(*router, unchanged, {}, true, false, true), RouterResultCode::NoError, ());
  TEST(UsesRoadType(unchanged.GetActive(), RoutingOptions::Ferry), ());
  RoutingOptions::SaveToSettings(VehicleType::Car, RoutingOptions(RoutingOptions::Ferry));
  RoutesResult after;
  TEST_EQUAL(Calculate(*router, after, {}, true, true, true), RouterResultCode::NoError, ());
  for (auto const & route : after.m_routes)
    TEST(!UsesRoadType(route, RoutingOptions::Ferry), ());
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
  TEST(UsesRoadType(result.GetActive(), RoutingOptions::Ferry), ());
  auto graph = router->MakeSingleMwmWorldGraph();
  TEST(!graph->IsRoutingOptionsGood(Segment(0, m_ferryFeature, 0, true)), ());
}
}  // namespace routing_preference_tests
