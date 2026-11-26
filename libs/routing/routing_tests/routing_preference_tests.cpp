#include "testing/testing.hpp"

#include "generator/altitude_generator.hpp"
#include "generator/generator_tests_support/test_generator.hpp"

#include "routing/cross_mwm_connector_serialization.hpp"
#include "routing/cross_mwm_graph.hpp"
#include "routing/data_source.hpp"
#include "routing/index_router.hpp"
#include "routing/route.hpp"
#include "routing/routing_options.hpp"
#include "routing/transit_graph.hpp"
#include "routing/transit_world_graph.hpp"
#include "routing/world_graph.hpp"

#include "routing_common/pedestrian_model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"

#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/files_container.hpp"

#include "base/file_name_utils.hpp"

#include "traffic/traffic_cache.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <map>
#include <utility>

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

  explicit RoutingPreferenceFixture(std::string const & fixture = "routing_options.osm",
                                    AltitudeGetter * altitudes = nullptr)
  {
    BuildFB(GetPlatform().ReadPathForFile("test_data/osm/" + fixture), "Germany");
    BuildFeatures("Germany");
    BuildRouting("Germany", "Germany");
    BuildCrossMwm("Germany", "Germany");
    if (altitudes)
      BuildRoadAltitudes(GetMwmPath("Germany"), *altitudes);
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

  std::unique_ptr<IndexRouter> Router(VehicleType vehicle, bool loadAltitudes = false)
  {
    return std::make_unique<IndexRouter>(vehicle, loadAltitudes, [](std::string const &) { return std::string(); },
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
  explicit SurfaceRoutingFixture(AltitudeGetter * altitudes = nullptr)
    : RoutingPreferenceFixture("routing_surfaces.osm", altitudes)
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

  static RouterResultCode CalculateSurfaceChoice(IndexRouter & router, RoutesResult & result, double latitude,
                                                 bool alternatives = false)
  {
    return router.CalculateRoute(
        Checkpoints(mercator::FromLatLon(latitude, 1.01), mercator::FromLatLon(latitude, 1.14)), {}, true, alternatives,
        {}, result);
  }

private:
  std::map<std::string, uint32_t> m_features;
};

UNIT_TEST(Routing_PreservesRoadAltitudesAndNumericFallbacks)
{
  class RoadAltitudes : public AltitudeGetter
  {
  public:
    explicit RoadAltitudes(bool omitFinish) : m_omitFinish(omitFinish) {}

    geometry::Altitude GetAltitude(m2::PointD const & point) override
    {
      // The last road has the same numeric heights when its samples are replaced by the section minimum.
      if (m_omitFinish && point.x > 1.095)
        return geometry::kInvalidAltitude;
      return point.x < 1.02 ? 100 : 0;
    }

  private:
    bool m_omitFinish;
  };

  auto const calculate = [](SurfaceRoutingFixture & fixture, bool loadAltitudes)
  {
    auto router = fixture.Router(VehicleType::Bicycle, loadAltitudes);
    RoutesResult result;
    TEST_EQUAL(SurfaceRoutingFixture::CalculateRequiredRoad(*router, result), RouterResultCode::NoError, ());
    return Route(result.GetActive());
  };

  RoadAltitudes completeHeights(false);
  SurfaceRoutingFixture completeMap(&completeHeights);
  auto const complete = calculate(completeMap, true /* loadAltitudes */);
  TEST(complete.HaveAltitudes(), ());
  geometry::Altitudes altitudes;
  complete.GetAltitudes(altitudes);
  TEST_GREATER(altitudes.front(), 75, ());
  TEST_LESS(altitudes.front(), 85, ());
  TEST_EQUAL(altitudes.back(), 0, ());
  auto const disabled = calculate(completeMap, false /* loadAltitudes */);
  TEST(!disabled.HaveAltitudes(), ());
  disabled.GetAltitudes(altitudes);
  TEST(std::ranges::all_of(altitudes, [](auto altitude) { return altitude == geometry::kInvalidAltitude; }), ());

  RoadAltitudes partialHeights(true);
  SurfaceRoutingFixture partialMap(&partialHeights);
  auto const partial = calculate(partialMap, true /* loadAltitudes */);
  TEST(!partial.HaveAltitudes(), ());
  partial.GetAltitudes(altitudes);
  TEST_EQUAL(altitudes.front(), complete.GetSubrouteAttrs(0).GetStart().GetAltitude(), ());
  TEST(std::ranges::find(altitudes, 0) != altitudes.end(), ());
  TEST_EQUAL(altitudes.back(), geometry::kInvalidAltitude, ());
  TEST_EQUAL(partial.GetSubrouteAttrs(0).GetFinish().GetAltitude(), geometry::kInvalidAltitude, ());
  TEST_EQUAL(partial.GetPoly().GetPoints(), complete.GetPoly().GetPoints(), ());
  TEST_ALMOST_EQUAL_ABS(partial.GetTotalTimeSec(), complete.GetTotalTimeSec(), 1e-8, ());
}

UNIT_TEST(Routing_UsesAltitudeFromTheSelectedSnap)
{
  class RoadAltitudes : public AltitudeGetter
  {
  public:
    explicit RoadAltitudes(bool omitSideRoad) : m_omitSideRoad(omitSideRoad) {}

    geometry::Altitude GetAltitude(m2::PointD const & point) override
    {
      if (m_omitSideRoad && mercator::YToLat(point.y) > 2.00005)
        return geometry::kInvalidAltitude;
      return point.x < 1.05 ? 100 : 200;
    }

  private:
    bool const m_omitSideRoad;
  };

  auto const calculate = [](bool omitSideRoad)
  {
    RoadAltitudes altitudes(omitSideRoad);
    RoutingPreferenceFixture fixture("routing_altitude_snap.osm", &altitudes);
    auto router = fixture.Router(VehicleType::Bicycle, true /* loadAltitudes */);
    RoutesResult result;
    Checkpoints const checkpoints(mercator::FromLatLon(2, 1.01), mercator::FromLatLon(2, 1.09));
    TEST_EQUAL(router->CalculateRoute(checkpoints, {}, false, false, {}, result), RouterResultCode::NoError, ());
    return Route(result.GetActive());
  };

  auto const complete = calculate(false /* omitSideRoad */);
  auto const partial = calculate(true /* omitSideRoad */);
  TEST(partial.HaveAltitudes(), ());
  geometry::Altitudes expected, actual;
  complete.GetAltitudes(expected);
  partial.GetAltitudes(actual);
  TEST_EQUAL(actual, expected, ());
  TEST_EQUAL(actual.front(), 100, ());
  TEST_EQUAL(actual.back(), 200, ());
  TEST_EQUAL(partial.GetPoly().GetPoints(), complete.GetPoly().GetPoints(), ());
  TEST_ALMOST_EQUAL_ABS(partial.GetTotalTimeSec(), complete.GetTotalTimeSec(), 1e-8, ());
}

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

UNIT_CLASS_TEST(SurfaceRoutingFixture, BicyclePreferencePricesMappedSurfacesWithoutChangingEta)
{
  using enum BicycleSurfaceBias;
  using enum EdgeEstimator::Purpose;
  struct SurfaceCase
  {
    char const * m_name;
    bool m_penalizedWhenPaved;
    bool m_penalizedWhenUnpaved;
    double m_baselineWeight = 0.0;
    double m_baselineEta = 0.0;
  };
  SurfaceCase cases[] = {{"Paved track", false, true},
                         {"Rough paved track", false, true},
                         {"Unpaved cycleway", true, false},
                         {"Bad unpaved cycleway", true, false},
                         {"Unknown track", false, false}};
  auto router = Router(VehicleType::Bicycle);
  for (auto const preference : {Any, PreferPaved, PreferUnpaved})
  {
    bool checked = false;
    m_onCountryLookup = [&]
    {
      if (checked)
        return;
      checked = true;
      auto graph = router->MakeSingleMwmWorldGraph();
      for (auto & test : cases)
      {
        auto const segment = NamedSegment(test.m_name);
        auto const weight = graph->CalcSegmentWeight(segment, Weight).GetWeight();
        auto const eta = graph->CalcSegmentWeight(segment, ETA).GetWeight();
        if (preference == Any)
        {
          test.m_baselineWeight = weight;
          test.m_baselineEta = eta;
        }
        else
        {
          bool const penalized = preference == PreferPaved ? test.m_penalizedWhenPaved : test.m_penalizedWhenUnpaved;
          if (penalized)
            TEST_GREATER(weight, test.m_baselineWeight, (test.m_name, preference));
          else
            TEST_EQUAL(weight, test.m_baselineWeight, (test.m_name, preference));
          TEST_EQUAL(eta, test.m_baselineEta, (test.m_name, preference));
        }
      }
    };
    RouterDelegate const delegate{RoutePlanningOptions{{}, preference}};
    RoutesResult result;
    TEST_EQUAL(Calculate(*router, result, delegate), RouterResultCode::NoError, (preference));
    TEST(checked, (preference));
  }
}

UNIT_CLASS_TEST(SurfaceRoutingFixture, BicycleSurfaceChoiceChangesTheRouteAndKeepsRequiredConnectors)
{
  using enum BicycleSurfacePreference;
  auto router = Router(VehicleType::Bicycle);

  for (auto const preference : {Any, PreferPaved, PreferUnpaved, AvoidUnpaved})
  {
    RoutePlanningOptions::SaveBicycleSurfacePreference(preference);
    RoutesResult result;
    TEST_EQUAL(CalculateSurfaceChoice(*router, result, 4, true /* alternatives */), RouterResultCode::NoError,
               (preference));
    TEST_EQUAL(UsesRoadType(result.GetActive(), RoutingOptions::Dirty),
               preference == Any || preference == PreferUnpaved, (preference));
    if (preference == AvoidUnpaved)
      for (auto const & route : result.m_routes)
        TEST(!UsesRoadType(route, RoutingOptions::Dirty), ());
  }
  RoutePlanningOptions::SaveBicycleSurfacePreference(PreferPaved);
  RoutesResult required;
  TEST_EQUAL(CalculateRequiredRoad(*router, required), RouterResultCode::NoError, ());
  TEST(UsesRoadType(required.GetActive(), RoutingOptions::Dirty), ());
}

UNIT_CLASS_TEST(SurfaceRoutingFixture, BicyclePreferUnpavedCanChooseALongerGravelRoute)
{
  auto router = Router(VehicleType::Bicycle);
  RoutesResult any;
  TEST_EQUAL(CalculateSurfaceChoice(*router, any, 5), RouterResultCode::NoError, ());
  TEST(!UsesRoadType(any.GetActive(), RoutingOptions::Dirty), ());

  RoutePlanningOptions::SaveBicycleSurfacePreference(BicycleSurfacePreference::PreferUnpaved);
  RoutesResult preferUnpaved;
  TEST_EQUAL(CalculateSurfaceChoice(*router, preferUnpaved, 5), RouterResultCode::NoError, ());
  TEST(UsesRoadType(preferUnpaved.GetActive(), RoutingOptions::Dirty), ());
  TEST_GREATER(preferUnpaved.GetActive().GetRouteSegments().back().GetDistFromBeginningMeters(),
               any.GetActive().GetRouteSegments().back().GetDistFromBeginningMeters(), ());
}

UNIT_CLASS_TEST(SurfaceRoutingFixture, BicycleSurfaceChoiceKeepsTheSelectedVariant)
{
  auto router = Router(VehicleType::Bicycle);
  RoutesResult initial;
  TEST_EQUAL(CalculateSurfaceChoice(*router, initial, 6, true /* alternatives */), RouterResultCode::NoError, ());
  TEST_EQUAL(initial.m_routes.size(), 2, ());
  TEST(!UsesRoadType(initial.GetActive(), RoutingOptions::Dirty), ());
  TEST(UsesRoadType(initial.m_routes[1], RoutingOptions::Dirty), ());
  router->SwapAltRouteToActive();

  RoutePlanningOptions::SaveBicycleSurfacePreference(BicycleSurfacePreference::PreferUnpaved);
  RoutesResult rebuilt;
  TEST_EQUAL(CalculateSurfaceChoice(*router, rebuilt, 6), RouterResultCode::NoError, ());
  TEST(UsesRoadType(rebuilt.GetActive(), RoutingOptions::Dirty), ());

  // With Prefer unpaved still saved, the Normal variant takes the paved route.
  router->ClearState();
  RoutesResult normal;
  TEST_EQUAL(CalculateSurfaceChoice(*router, normal, 6), RouterResultCode::NoError, ());
  TEST(!UsesRoadType(normal.GetActive(), RoutingOptions::Dirty), ());
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

UNIT_CLASS_TEST(RoutingPreferenceFixture, TransitModeFilterAlsoExcludesRoadFerries)
{
  auto router = Router(VehicleType::Transit);
  auto const usesFerry = [this](RouteBase const & route)
  {
    return std::ranges::any_of(route.GetRouteSegments(), [this](RouteSegment const & segment)
    { return segment.GetSegment().GetFeatureId() == m_ferryFeature; });
  };

  // Try crossing the ferry and starting on a partial ferry segment away from either pier.
  struct
  {
    double m_start;
    double m_finish;
    RouterResultCode m_restrictedResult;
  } const endpoints[] = {
      {1.0995, 1.4005, RouterResultCode::TransitRouteNotFoundNoNetwork},
      {1.25, 1.0995, RouterResultCode::StartPointNotFound},
  };
  for (auto const & [start, finish, restrictedResult] : endpoints)
  {
    Checkpoints const checkpoints(mercator::FromLatLon(1, start), mercator::FromLatLon(1, finish));
    RoutesResult allowed;
    TEST_EQUAL(router->CalculateRoute(checkpoints, {}, false, false, RouterDelegate(RoutePlanningOptions{}), allowed),
               RouterResultCode::NoError, (start, finish));
    TEST(usesFerry(allowed.GetActive()), (start, finish));

    RoutePlanningOptions options;
    options.m_transitModes = static_cast<transit::ModeMask>(transit::Mode::Bus);
    RoutesResult restricted;
    TEST_EQUAL(router->CalculateRoute(checkpoints, {}, false, false, RouterDelegate(options), restricted),
               restrictedResult, (start, finish));
    TEST(restricted.m_routes.empty(), (start, finish));
  }
}

UNIT_CLASS_TEST(RoutingPreferenceFixture, TransitGateEndingsHonorRoadFerryExclusion)
{
  transit::Gate gate(1, 0, true, true, 0, {1}, mercator::FromLatLon(1, 1.2));
  gate.SetBestPedestrianSegment({m_ferryFeature, 0, true});
  auto router = Router(VehicleType::Pedestrian);
  for (bool const avoid : {false, true})
  {
    RoutingOptions::SaveToSettings(VehicleType::Pedestrian, RoutingOptions(avoid ? RoutingOptions::Ferry : 0));
    auto graph = router->MakeSingleMwmWorldGraph();
    TransitGraph::Endings endings;
    MakeGateEndings({gate}, 0, graph->GetIndexGraph(0), endings);
    TEST_EQUAL(endings.contains(gate.GetOsmId()), !avoid, (avoid));
  }
}

UNIT_TEST(TransitModeFilterKeepsRoadSnapsLocalNearExcludedFerries)
{
  RoutingPreferenceFixture fixture("routing_ferry_snapping.osm");
  auto router = fixture.Router(VehicleType::Transit);
  auto const onLand = mercator::FromLatLon(1, 1.0009);
  auto const onFerry = mercator::FromLatLon(1, 1.0025);
  auto const nearPier = mercator::FromLatLon(1, 1.0012);
  auto const calculate =
      [&](m2::PointD const & start, m2::PointD const & finish, transit::ModeMask modes, RoutesResult & routes)
  {
    RoutePlanningOptions options;
    options.m_transitModes = modes;
    return router->CalculateRoute(Checkpoints(start, finish), {}, false, false, RouterDelegate(options), routes);
  };

  RoutesResult allowed;
  TEST_EQUAL(calculate(onFerry, onLand, transit::kAllModes, allowed), RouterResultCode::NoError, ());
  auto const buses = static_cast<transit::ModeMask>(transit::Mode::Bus);
  RoutesResult restricted;
  TEST_EQUAL(calculate(onFerry, onLand, buses, restricted), RouterResultCode::StartPointNotFound, ());
  TEST_EQUAL(calculate(onLand, onFerry, buses, restricted), RouterResultCode::EndPointNotFound, ());

  TEST_EQUAL(calculate(nearPier, onLand, buses, restricted), RouterResultCode::NoError, ());
  auto const & route = restricted.GetActive();
  TEST_LESS(route.GetRouteSegments().back().GetDistFromBeginningMeters(), 40.0, ());
  TEST(std::ranges::none_of(route.GetRouteSegments(), [&](RouteSegment const & segment)
  { return segment.GetSegment().GetFeatureId() == fixture.m_ferryFeature; }),
       ());
}

UNIT_TEST(TransitModeFilterChecksTheRoadTwinInItsOwnMap)
{
  generator::tests_support::TestRawGenerator generator;
  auto const original = GetPlatform().ReadPathForFile("test_data/osm/routing_options.osm");
  std::string xml;
  FileReader(original).ReadAsString(xml);
  std::string_view constexpr ferryTag = R"(<tag k="route" v="ferry" />)";
  xml.replace(xml.find(ferryTag), ferryTag.size(), R"(<tag k="highway" v="residential" />)");
  auto const oldInput = base::JoinPath(generator.GetGenInfo().m_cacheDir, "road-before-ferry.osm");
  FileWriter(oldInput).Write(xml.data(), xml.size());

  FrozenDataSource data;
  auto ids = std::make_shared<NumMwmIds>();
  auto tree = std::make_shared<m4::Tree<NumMwmId>>();
  auto const osmWay = base::MakeOsmWay(11);
  m2::RectD bounds;
  auto const makeMap = [&](std::string const & name, std::string const & input, bool forwardIsEnter)
  {
    generator.BuildFB(input, name);
    generator.BuildFeatures(name);
    generator.BuildRouting(name, name);
    auto const features = generator.LoadFID2OsmID(name);
    auto const feature = std::ranges::find_if(features, [&](auto const & entry) { return entry.second == osmWay; });
    TEST(feature != features.end(), (name));
    auto const path = generator.GetMwmPath(name);
    {
      CrossMwmConnectorBuilderEx<base::GeoObjectId> transitions;
      transitions.AddTransition(osmWay, feature->first, 0, kPedestrianMask, 0, forwardIsEnter);
      FilesContainerW map(path, FileWriter::OP_WRITE_EXISTING);
      transitions.Serialize(*map.GetWriter(CROSS_MWM_FILE_TAG));
    }
    auto const file = platform::LocalCountryFile::MakeTemporary(path);
    auto const [id, status] = data.RegisterMap(file);
    TEST_EQUAL(status, MwmSet::RegResult::Success, (name));
    ids->RegisterFile(file.GetCountryFile());
    auto const mwmId = ids->GetId(file.GetCountryFile());
    bounds = data.GetMwmHandleById(id).GetValue()->GetHeader().GetBounds();
    tree->Add(mwmId, bounds);
    return Segment(mwmId, feature->first, 0, true);
  };
  auto const before = makeMap("Germany", oldInput, false);
  auto const after = makeMap("Austria", original, true);

  MwmDataSource source(data, ids);
  CountryRectFn const countryRect = [&](std::string const &) { return bounds; };
  auto crossMwm = std::make_unique<CrossMwmGraph>(ids, tree, VehicleType::Pedestrian, countryRect, source);
  auto estimator = EdgeEstimator::Create(VehicleType::Pedestrian, 60, SpeedKMpH(3), nullptr, nullptr, nullptr);
  auto index = IndexGraphLoader::Create(VehicleType::Pedestrian, false, std::make_shared<PedestrianModelFactory>(),
                                        estimator, source, RoutingOptions(RoutingOptions::Ferry));
  auto transit = TransitGraphLoader::Create(source, estimator, static_cast<transit::ModeMask>(transit::Mode::Bus));
  TransitWorldGraph graph(std::move(crossMwm), std::move(index), std::move(transit), estimator);
  TEST(graph.IsRoutingOptionsGood(before), ());
  TEST(!graph.IsRoutingOptionsGood(after), ());
  for (bool const filter : {false, true})
  {
    WorldGraph::SegmentEdgeListT edges;
    graph.GetEdgeList(before, true /* isOutgoing */, filter, edges);
    TEST_EQUAL(
        std::ranges::any_of(edges, [&](auto const & edge) { return edge.GetTarget().GetMwmId() == after.GetMwmId(); }),
        !filter, (filter));
  }
}

UNIT_CLASS_TEST(RoutingPreferenceFixture, TransitRoadFerryKeepsItsWeightOutsideTheWalkingBudget)
{
  using enum EdgeEstimator::Purpose;
  auto router = Router(VehicleType::Transit);
  auto graph = router->MakeSingleMwmWorldGraph();
  Segment const ferry(0, m_ferryFeature, 0, true);
  auto const & road = graph->GetIndexGraph(ferry.GetMwmId()).GetRoadGeometry(ferry.GetFeatureId());
  TEST(road.GetRoutingOptions().Has(RoutingOptions::Ferry), ());
  auto estimator = EdgeEstimator::Create(VehicleType::Transit, 60, SpeedKMpH(3), nullptr, nullptr, nullptr);
  auto const eta = estimator->CalcRoadSegmentWeight(ferry, road, ETA);
  TEST_GREATER(eta.GetWeight(), 50 * 60, ());

  estimator->SetTransitPreference(TransitPreference::LessWalking);
  auto const weight = estimator->CalcRoadSegmentWeight(ferry, road, Weight);
  TEST_EQUAL(weight.GetWeight(), eta.GetWeight(), ());
  TEST_EQUAL(weight.GetWalkingTime(), 0.0, ());
  TEST_EQUAL(weight.GetTransitTime(), 0.0, ());
  TEST(graph->CheckLength(weight, 0), ());
}

UNIT_CLASS_TEST(SurfaceRoutingFixture, TransitWalkingBudgetUsesPhysicalRoadTime)
{
  using enum EdgeEstimator::Purpose;
  auto router = Router(VehicleType::Transit);
  auto graph = router->MakeSingleMwmWorldGraph();
  auto const segment = NamedSegment("Paved connection");
  auto const & road = graph->GetIndexGraph(segment.GetMwmId()).GetRoadGeometry(segment.GetFeatureId());
  auto estimator = EdgeEstimator::Create(VehicleType::Transit, 60, SpeedKMpH(3), nullptr, nullptr, nullptr);
  auto const eta = estimator->CalcRoadSegmentWeight(segment, road, ETA).GetWeight();
  auto const recommended = estimator->CalcRoadSegmentWeight(segment, road, Weight);
  TEST_NOT_EQUAL(recommended.GetWeight(), eta, ());

  estimator->SetTransitPreference(TransitPreference::LessWalking);
  auto const lessWalking = estimator->CalcRoadSegmentWeight(segment, road, Weight);
  TEST_GREATER(lessWalking.GetWeight(), recommended.GetWeight(), ());
  TEST_ALMOST_EQUAL_ABS(recommended.GetWalkingTime(), eta, 1e-4, ());
  TEST_EQUAL(lessWalking.GetWalkingTime(), recommended.GetWalkingTime(), ());
}

class TransitPreferenceFixture : public RoutingPreferenceFixture
{
public:
  TransitPreferenceFixture() : RoutingPreferenceFixture("routing_transit_preferences.osm") {}
};

UNIT_CLASS_TEST(TransitPreferenceFixture, SelectedTransitVariantSurvivesRebuild)
{
  auto router = Router(VehicleType::Transit);
  Checkpoints const checkpoints(mercator::FromLatLon(1, 1.0005), mercator::FromLatLon(1, 1.0105));
  auto const takesFerry = [this](RouteBase const & route)
  {
    return std::ranges::any_of(route.GetRouteSegments(), [this](RouteSegment const & segment)
    { return segment.GetSegment().GetFeatureId() == m_ferryFeature; });
  };
  auto const calculate = [&](TransitPreference preference, bool alternatives)
  {
    RoutePlanningOptions options;
    options.m_transitPreference = preference;
    RoutesResult result;
    TEST_EQUAL(router->CalculateRoute(checkpoints, {}, false, alternatives, RouterDelegate(options), result),
               RouterResultCode::NoError, ());
    return result;
  };

  using enum TransitPreference;
  auto const initial = calculate(LessWalking, true);
  TEST_EQUAL(initial.m_routes.size(), 2, ());
  TEST(takesFerry(initial.GetActive()), ());
  TEST(!takesFerry(initial.m_routes[1]), ());
  router->SwapAltRouteToActive();
  TEST(!takesFerry(calculate(LessWalking, false).GetActive()), ());

  router->ClearState();
  TEST(takesFerry(calculate(LessWalking, true).GetActive()), ());
  router->SwapAltRouteToActive();
  TEST(!takesFerry(calculate(Recommended, false).GetActive()), ());
}

class StepsRoutingFixture : public RoutingPreferenceFixture
{
public:
  StepsRoutingFixture() : RoutingPreferenceFixture("routing_steps.osm") {}
};

UNIT_CLASS_TEST(StepsRoutingFixture, StepsAvoidanceIsIndependent)
{
  for (auto const avoided : {VehicleType::Bicycle, VehicleType::Pedestrian})
  {
    RoutingOptions::SaveToSettings(VehicleType::Bicycle, {});
    RoutingOptions::SaveToSettings(VehicleType::Pedestrian, {});
    RoutingOptions::SaveToSettings(avoided, RoutingOptions(RoutingOptions::Steps));
    for (auto const vehicle : {VehicleType::Bicycle, VehicleType::Pedestrian})
    {
      auto router = Router(vehicle);
      RoutesResult result;
      TEST_EQUAL(Calculate(*router, result, {}, false, true), RouterResultCode::NoError, (vehicle));
      TEST_EQUAL(UsesRoadType(result.GetActive(), RoutingOptions::Steps), vehicle != avoided, (vehicle, avoided));
      if (vehicle == avoided)
        for (auto const & route : result.m_routes)
          TEST(!UsesRoadType(route, RoutingOptions::Steps), (vehicle));
    }
  }
}

UNIT_CLASS_TEST(StepsRoutingFixture, SubmittedOptionsOverrideSettingsForTheWholeRequest)
{
  auto const vehicle = VehicleType::Pedestrian;
  auto router = Router(vehicle);
  for (bool const avoid : {false, true, false})
  {
    RoutingOptions::SaveToSettings(vehicle, RoutingOptions(avoid ? 0 : RoutingOptions::Steps));
    RouterDelegate const delegate{RoutePlanningOptions{RoutingOptions(avoid ? RoutingOptions::Steps : 0)}};
    RoutesResult result;
    TEST_EQUAL(Calculate(*router, result, delegate, true, true), RouterResultCode::NoError, (avoid));
    TEST_EQUAL(UsesRoadType(result.GetActive(), RoutingOptions::Steps), !avoid, (avoid));
  }
}

UNIT_CLASS_TEST(StepsRoutingFixture, RequiredStepsFailTheRoute)
{
  // Small isolated approaches fail endpoint snapping; larger components reach the path search.
  std::pair<double, RouterResultCode> const cases[] = {{2, RouterResultCode::StartPointNotFound},
                                                       {3, RouterResultCode::RouteNotFound}};
  for (auto const vehicle : {VehicleType::Bicycle, VehicleType::Pedestrian})
  {
    for (auto const & [latitude, expected] : cases)
    {
      RoutingOptions::SaveToSettings(vehicle, {});
      auto router = Router(vehicle);
      auto const calculate = [&](RoutesResult & result)
      {
        return router->CalculateRoute(
            Checkpoints(mercator::FromLatLon(latitude, 1.01), mercator::FromLatLon(latitude, 1.09)), {}, false, true,
            {}, result);
      };
      RoutesResult before;
      TEST_EQUAL(calculate(before), RouterResultCode::NoError, (vehicle, latitude));
      TEST(UsesRoadType(before.GetActive(), RoutingOptions::Steps), (vehicle, latitude));
      RoutingOptions::SaveToSettings(vehicle, RoutingOptions(RoutingOptions::Steps));
      RoutesResult after;
      TEST_EQUAL(calculate(after), expected, (vehicle, latitude));
      TEST(after.m_routes.empty(), (vehicle, latitude));
    }
  }
}

class TrailDifficultyFixture : public RoutingPreferenceFixture
{
public:
  TrailDifficultyFixture() : RoutingPreferenceFixture("routing_trail_difficulty.osm") {}
};

UNIT_CLASS_TEST(TrailDifficultyFixture, WalkingRouteRetainsTrailGrades)
{
  auto router = Router(VehicleType::Pedestrian);
  RoutesResult result;
  TEST_EQUAL(Calculate(*router, result), RouterResultCode::NoError, ());
  auto const & route = result.GetActive();
  // The direct route uses both graded trails even though each has an ordinary detour.
  TEST(UsesRoadType(route, RoutingOptions::DifficultTrail), ());
  TEST(UsesRoadType(route, RoutingOptions::ExpertTrail), ());

  bool unpavedDifficultTrail = false;
  for (auto const & segment : route.GetRouteSegments())
  {
    auto const types = segment.GetRoadTypes();
    if (!types.Has(RoutingOptions::DifficultTrail))
      continue;
    TEST(!types.Has(RoutingOptions::ExpertTrail), ());
    unpavedDifficultTrail |= types.Has(RoutingOptions::Dirty);
  }
  // Trail classification preserves the separately mapped gravel surface.
  TEST(unpavedDifficultTrail, ());
}

UNIT_CLASS_TEST(TrailDifficultyFixture, WalkingCanAvoidBothTrailGrades)
{
  auto router = Router(VehicleType::Pedestrian);
  // The expert trail ends 16 m from the start, inside the initial snapping radius. With avoidance on,
  // the route should start on an ordinary path and take the detour around each grade.
  Checkpoints const checkpoints(mercator::FromLatLon(0.9999, 1.1401), mercator::FromLatLon(1, 1.01));
  for (bool const avoid : {false, true, false})
  {
    RoutingOptions::SaveToSettings(VehicleType::Pedestrian, RoutingOptions(avoid ? RoutingOptions::DifficultTrail : 0));
    RoutesResult result;
    TEST_EQUAL(router->CalculateRoute(checkpoints, {}, true, true, {}, result), RouterResultCode::NoError, (avoid));
    for (auto const grade : {RoutingOptions::DifficultTrail, RoutingOptions::ExpertTrail})
    {
      if (avoid)
        for (auto const & variant : result.m_routes)
          TEST(!UsesRoadType(variant, grade), (grade));
      else
        TEST(UsesRoadType(result.GetActive(), grade), (grade));
    }
  }
}

UNIT_CLASS_TEST(TrailDifficultyFixture, WalkingStartOnAvoidedTrailKeepsPartialSegment)
{
  RoutingOptions::SaveToSettings(VehicleType::Pedestrian, RoutingOptions(RoutingOptions::DifficultTrail));
  auto router = Router(VehicleType::Pedestrian);
  struct Trail
  {
    RoutingOptions::Road grade;
    double midpointLon;
    double exitLon;
  };
  // Walk from the middle of each graded way to its junction with an ordinary path.
  Trail const trails[] = {{RoutingOptions::DifficultTrail, 1.11, 1.10}, {RoutingOptions::ExpertTrail, 1.13, 1.14}};
  for (auto const & [grade, midpointLon, exitLon] : trails)
  {
    auto const onTrail = mercator::FromLatLon(1, midpointLon);
    auto const onPath = mercator::FromLatLon(1, exitLon);
    // Try the trail itself and points 39 m and 41 m away, on either side of the initial search radius.
    for (double const latitude : {1.0, 0.99965, 0.99963})
    {
      auto const start = mercator::FromLatLon(latitude, midpointLon);
      RoutesResult result;
      TEST_EQUAL(router->CalculateRoute(Checkpoints(start, onPath), {}, false, false, {}, result),
                 RouterResultCode::NoError, (grade, latitude));
      auto const & segments = result.GetActive().GetRouteSegments();
      auto const trail =
          std::ranges::find_if(segments, [grade](auto const & s) { return s.GetRoadTypes().Has(grade); });
      TEST(trail != segments.end(), (grade, latitude));
      double const approachM = trail == segments.begin() ? 0.0 : std::prev(trail)->GetDistFromBeginningMeters();
      TEST_ALMOST_EQUAL_ABS(approachM, mercator::DistanceOnEarth(start, onTrail), 0.5, (grade, latitude));
    }
  }
}

UNIT_CLASS_TEST(TrailDifficultyFixture, RequiredExpertTrailFailsWithAvoidanceOn)
{
  auto router = Router(VehicleType::Pedestrian);
  auto const calculate = [&](RoutesResult & result)
  {
    return router->CalculateRoute(Checkpoints(mercator::FromLatLon(2, 1.01), mercator::FromLatLon(2, 1.09)), {}, false,
                                  true, {}, result);
  };
  RoutesResult before;
  TEST_EQUAL(calculate(before), RouterResultCode::NoError, ());
  TEST(UsesRoadType(before.GetActive(), RoutingOptions::ExpertTrail), ());

  RoutingOptions::SaveToSettings(VehicleType::Pedestrian, RoutingOptions(RoutingOptions::DifficultTrail));
  RoutesResult after;
  // A required expert trail currently produces a generic endpoint error.
  TEST_EQUAL(calculate(after), RouterResultCode::StartPointNotFound, ());
  TEST(after.m_routes.empty(), ());
}
}  // namespace routing_preference_tests
