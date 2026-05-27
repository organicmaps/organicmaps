#include "testing/testing.hpp"

#include "routing/bike_sharing_heuristic.hpp"
#include "routing/data_source.hpp"
#include "routing/index_router.hpp"
#include "routing/route.hpp"
#include "routing/router_delegate.hpp"
#include "routing/routing_options.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_with_custom_mwms.hpp"
#include "generator/routing_index_generator.hpp"

#include "traffic/traffic_cache.hpp"

#include "geometry/mercator.hpp"

#include "platform/local_country_file.hpp"

#include <memory>
#include <string>
#include <vector>

namespace bike_sharing_heuristic_tests
{
using namespace generator::tests_support;
using namespace routing;

TestPOI MakeBicycleRentalStation(m2::PointD const & point, std::string const & name)
{
  TestPOI station(point, name, "en");
  station.SetTypes({{"amenity", "bicycle_rental"}});
  return station;
}

class BikeSharingHeuristicTest : public TestWithCustomMwms
{
public:
  MwmDataSource MakeRoutingDataSource() { return {m_dataSource, nullptr}; }

  void BuildRoutingSection(std::string const & countryName)
  {
    auto const parentGetter = [](std::string const &) { return std::string(); };
    TEST(routing_builder::BuildRoutingIndex(m_files.back().GetPath(MapFileType::Map), countryName, parentGetter), ());

    m_dataSource.DeregisterMap(m_files.back().GetCountryFile());
    auto const result = m_dataSource.RegisterMap(m_files.back());
    CHECK_EQUAL(result.second, MwmSet::RegResult::Success, ());
    CHECK(result.first.GetInfo(), ());
  }

  IndexRouter MakeBicycleRouter(std::string const & countryName, m2::RectD const & countryRect)
  {
    auto numMwmIds = std::make_shared<NumMwmIds>();
    platform::CountryFile const countryFile(countryName);
    numMwmIds->RegisterFile(countryFile);

    auto numMwmTree = std::make_shared<m4::Tree<NumMwmId>>();
    numMwmTree->Add(numMwmIds->GetId(countryFile), countryRect);

    CountryParentNameGetterFn const parentGetter = [](std::string const &) { return std::string(); };
    IndexRouter::TCountryFileFn const countryFileGetter = [countryName](m2::PointD const &) { return countryName; };
    CountryRectFn const countryRectGetter = [countryRect](std::string const &) { return countryRect; };

    return {VehicleType::Bicycle, true /* loadAltitudes */, parentGetter,
            countryFileGetter,    countryRectGetter,        numMwmIds,
            numMwmTree,           m_trafficCache,           m_dataSource};
  }

private:
  traffic::TrafficCache m_trafficCache;
};

class BicycleOptionsGuard
{
public:
  explicit BicycleOptionsGuard(RoutingOptions options) : m_saved(RoutingOptions::LoadBicycleOptionsFromSettings())
  {
    RoutingOptions::SaveBicycleOptionsToSettings(options);
  }

  ~BicycleOptionsGuard() { RoutingOptions::SaveBicycleOptionsToSettings(m_saved); }

private:
  RoutingOptions m_saved;
};

UNIT_CLASS_TEST(BikeSharingHeuristicTest, NearestStationsAreLimitedAndSortedByDistance)
{
  m2::PointD const center = mercator::FromLatLon(0.0, 0.0);
  std::vector<BicycleRentalStation> stations = {{mercator::GetSmPoint(center, 900.0, 0.0), "station-900"},
                                                {mercator::GetSmPoint(center, 100.0, 0.0), "station-100"},
                                                {mercator::GetSmPoint(center, 500.0, 0.0), "station-500"},
                                                {mercator::GetSmPoint(center, 300.0, 0.0), "station-300"}};
  auto const nearest = GetNearestStations(stations, center);

  TEST_EQUAL(kPublicBicycleMaxStationsPerSide, 3, ());
  TEST_EQUAL(nearest.size(), kPublicBicycleMaxStationsPerSide, ());
  TEST_EQUAL(nearest[0].m_provider, "station-100", ());
  TEST_EQUAL(nearest[1].m_provider, "station-300", ());
  TEST_EQUAL(nearest[2].m_provider, "station-500", ());
}

UNIT_CLASS_TEST(BikeSharingHeuristicTest, FoundStationsCanBeLimitedAndSortedByDistance)
{
  m2::PointD const center = mercator::FromLatLon(0.0, 0.0);

  auto station900 = MakeBicycleRentalStation(mercator::GetSmPoint(center, 900.0, 0.0), "station-900");
  auto station100 = MakeBicycleRentalStation(mercator::GetSmPoint(center, 100.0, 0.0), "station-100");
  auto station500 = MakeBicycleRentalStation(mercator::GetSmPoint(center, 500.0, 0.0), "station-500");
  auto station300 = MakeBicycleRentalStation(mercator::GetSmPoint(center, 300.0, 0.0), "station-300");
  auto stationOutsideRadius =
      MakeBicycleRentalStation(mercator::GetSmPoint(center, 2000.0, 0.0), "station-outside-radius");

  BuildCountry("BikeSharingNearest", [&](TestMwmBuilder & builder)
  {
    builder.Add(station900);
    builder.Add(station100);
    builder.Add(station500);
    builder.Add(station300);
    builder.Add(stationOutsideRadius);
  });

  auto routingDataSource = MakeRoutingDataSource();
  auto const stations =
      GetNearestStations(FindBicycleRentals(routingDataSource, center, kPublicBicycleMaxWalkRadiusM), center);

  TEST_EQUAL(stations.size(), kPublicBicycleMaxStationsPerSide, ());
  TEST_ALMOST_EQUAL_ABS(mercator::DistanceOnEarth(center, stations[0].m_point), 100.0, 1.0, ());
  TEST_ALMOST_EQUAL_ABS(mercator::DistanceOnEarth(center, stations[1].m_point), 300.0, 1.0, ());
  TEST_ALMOST_EQUAL_ABS(mercator::DistanceOnEarth(center, stations[2].m_point), 500.0, 1.0, ());
}

UNIT_CLASS_TEST(BikeSharingHeuristicTest, FindBicycleRentalsRespectsWalkRadius)
{
  m2::PointD const center = mercator::FromLatLon(0.0, 0.0);

  auto stationInRadius = MakeBicycleRentalStation(mercator::GetSmPoint(center, 200.0, 0.0), "station-in-radius");

  auto stationOutsideRadius =
      MakeBicycleRentalStation(mercator::GetSmPoint(center, 2000.0, 0.0), "station-outside-radius");

  // Add a non-station POI within the radius to check that it is not returned as a station
  TestPOI cafe(mercator::GetSmPoint(center, 100.0, 0.0), "cafe", "en");
  cafe.SetTypes({{"amenity", "cafe"}});

  BuildCountry("BikeSharingRadius", [&](TestMwmBuilder & builder)
  {
    builder.Add(stationInRadius);
    builder.Add(stationOutsideRadius);
    builder.Add(cafe);
  });

  auto routingDataSource = MakeRoutingDataSource();
  auto const stations = FindBicycleRentals(routingDataSource, center, kPublicBicycleMaxWalkRadiusM);

  TEST_EQUAL(kPublicBicycleMaxWalkRadiusM, 1500.0, ());
  TEST_EQUAL(stations.size(), 1, ());
  // Check that the station within the radius is returned and the one outside is not
  TEST_LESS_OR_EQUAL(mercator::DistanceOnEarth(center, stations.front().m_point), kPublicBicycleMaxWalkRadiusM, ());
}

UNIT_CLASS_TEST(BikeSharingHeuristicTest, FindBicycleRentalsPrefersNetworkMetadata)
{
  m2::PointD const center = mercator::FromLatLon(0.0, 0.0);

  auto station = MakeBicycleRentalStation(center, "station");
  station.GetMetadata().Set(feature::Metadata::FMD_NETWORK, "network-name");
  station.GetMetadata().Set(feature::Metadata::FMD_OPERATOR, "operator-name");

  BuildCountry("BikeSharingProvider", [&](TestMwmBuilder & builder) { builder.Add(station); });

  auto routingDataSource = MakeRoutingDataSource();
  auto const stations = FindBicycleRentals(routingDataSource, center, kPublicBicycleMaxWalkRadiusM);

  TEST_EQUAL(stations.size(), 1, ());
  TEST_EQUAL(stations.front().m_provider, "network-name", ());
}

UNIT_CLASS_TEST(BikeSharingHeuristicTest, FindBicycleRentalsFallsBackToOperatorMetadata)
{
  m2::PointD const center = mercator::FromLatLon(0.0, 0.0);

  auto station = MakeBicycleRentalStation(center, "station");
  station.GetMetadata().Set(feature::Metadata::FMD_OPERATOR, "operator-name");

  BuildCountry("BikeSharingOperatorFallback", [&](TestMwmBuilder & builder) { builder.Add(station); });

  auto routingDataSource = MakeRoutingDataSource();
  auto const stations = FindBicycleRentals(routingDataSource, center, kPublicBicycleMaxWalkRadiusM);

  TEST_EQUAL(stations.size(), 1, ());
  TEST_EQUAL(stations.front().m_provider, "operator-name", ());
}

UNIT_CLASS_TEST(BikeSharingHeuristicTest, DifferentProvidersAreRejected)
{
  BicycleRentalStation const startStation{mercator::FromLatLon(0.0, 0.0), "provider-a"};
  BicycleRentalStation const finishStation{mercator::FromLatLon(0.0, 0.0), "provider-b"};

  TEST(!AreBicycleRentalStationsCompatible(startStation, finishStation), ());
}

UNIT_CLASS_TEST(BikeSharingHeuristicTest, SameProvidersAreAccepted)
{
  BicycleRentalStation const startStation{mercator::FromLatLon(0.0, 0.0), "provider-a"};
  BicycleRentalStation const finishStation{mercator::FromLatLon(0.0, 0.0), "provider-a"};

  TEST(AreBicycleRentalStationsCompatible(startStation, finishStation), ());
}

UNIT_CLASS_TEST(BikeSharingHeuristicTest, MissingProviderDoesNotBlockStationPair)
{
  BicycleRentalStation const startStation{mercator::FromLatLon(0.0, 0.0), ""};
  BicycleRentalStation const finishStation{mercator::FromLatLon(0.0, 0.0), "provider-a"};

  TEST(AreBicycleRentalStationsCompatible(startStation, finishStation), ());
}

UNIT_CLASS_TEST(BikeSharingHeuristicTest, FindBicycleRentalsReturnsEmptyWhenNoStationsNearby)
{
  m2::PointD const center = mercator::FromLatLon(0.0, 0.0);

  TestPOI cafe(mercator::GetSmPoint(center, 50.0, 0.0), "cafe", "en");
  cafe.SetTypes({{"amenity", "cafe"}});

  BuildCountry("BikeSharingEmpty", [&](TestMwmBuilder & builder) { builder.Add(cafe); });

  auto routingDataSource = MakeRoutingDataSource();
  auto const stations = FindBicycleRentals(routingDataSource, center, kPublicBicycleMaxWalkRadiusM);

  TEST(stations.empty(), ());
}

UNIT_CLASS_TEST(BikeSharingHeuristicTest, IndexRouterBuildsPublicBicycleRouteWithWalkingAndBicycleLegs)
{
  std::string const kCountryName = "BikeSharingRoute";

  m2::PointD const start = mercator::FromLatLon(0.0, 0.0);
  m2::PointD const startStation = mercator::GetSmPoint(start, 200.0, 0.0);
  m2::PointD const finishStation = mercator::GetSmPoint(start, 800.0, 0.0);
  m2::PointD const finish = mercator::GetSmPoint(start, 1000.0, 0.0);
  m2::RectD const countryRect = mercator::RectByCenterXYAndSizeInMeters(start, 3000.0);

  TestStreet street({start, startStation, finishStation, finish}, "bike sharing street", "en");

  auto startStationPoi = MakeBicycleRentalStation(startStation, "start-station");
  startStationPoi.GetMetadata().Set(feature::Metadata::FMD_NETWORK, "network");

  auto finishStationPoi = MakeBicycleRentalStation(finishStation, "finish-station");
  finishStationPoi.GetMetadata().Set(feature::Metadata::FMD_NETWORK, "network");

  BuildCountry(kCountryName, [&](TestMwmBuilder & builder)
  {
    builder.Add(street);
    builder.Add(startStationPoi);
    builder.Add(finishStationPoi);
  });
  BuildRoutingSection(kCountryName);

  RoutingOptions options;
  options.Add(RoutingOptions::Road::PublicBicycle);
  BicycleOptionsGuard const optionsGuard(options);

  auto router = MakeBicycleRouter(kCountryName, countryRect);
  RouterDelegate const delegate;
  Route route("public-bicycle-route", 0 /* routeId */);

  auto const result = router.CalculateRoute(Checkpoints(start, finish), m2::PointD::Zero() /* startDirection */,
                                            false /* adjustToPrevRoute */, delegate, route);

  TEST_EQUAL(result, RouterResultCode::NoError, ());
  TEST_EQUAL(route.GetSubrouteCount(), 3, ());
  TEST_EQUAL(route.GetSubrouteAttrs(0).GetVehicleType(), VehicleType::Pedestrian, ());
  TEST_EQUAL(route.GetSubrouteAttrs(1).GetVehicleType(), VehicleType::Bicycle, ());
  TEST_EQUAL(route.GetSubrouteAttrs(2).GetVehicleType(), VehicleType::Pedestrian, ());
  TEST_GREATER(route.GetTotalDistanceMeters(), 0.0, ());
}

UNIT_CLASS_TEST(BikeSharingHeuristicTest, IndexRouterRejectsPublicBicycleRouteWithSameStartAndFinishStation)
{
  std::string const kCountryName = "BikeSharingSameStationRoute";

  m2::PointD const start = mercator::FromLatLon(0.0, 0.0);
  m2::PointD const station = mercator::GetSmPoint(start, 200.0, 0.0);
  m2::PointD const finish = mercator::GetSmPoint(start, 400.0, 0.0);
  m2::RectD const countryRect = mercator::RectByCenterXYAndSizeInMeters(start, 3000.0);

  TestStreet street({start, station, finish}, "bike sharing street", "en");

  auto stationPoi = MakeBicycleRentalStation(station, "station");
  stationPoi.GetMetadata().Set(feature::Metadata::FMD_NETWORK, "network");

  BuildCountry(kCountryName, [&](TestMwmBuilder & builder)
  {
    builder.Add(street);
    builder.Add(stationPoi);
  });
  BuildRoutingSection(kCountryName);

  RoutingOptions options;
  options.Add(RoutingOptions::Road::PublicBicycle);
  BicycleOptionsGuard const optionsGuard(options);

  auto router = MakeBicycleRouter(kCountryName, countryRect);
  RouterDelegate const delegate;
  Route route("public-bicycle-route", 0 /* routeId */);

  auto const result = router.CalculateRoute(Checkpoints(start, finish), m2::PointD::Zero() /* startDirection */,
                                            false /* adjustToPrevRoute */, delegate, route);

  TEST_EQUAL(result, RouterResultCode::RouteNotFound, ());
}

UNIT_CLASS_TEST(BikeSharingHeuristicTest, IndexRouterRejectsPublicBicycleRouteWithLongWalkingLeg)
{
  std::string const kCountryName = "BikeSharingLongWalkingRoute";

  m2::PointD const start = mercator::FromLatLon(0.0, 0.0);
  m2::PointD const detour1 = mercator::GetSmPoint(start, 0.0, 1000.0);
  m2::PointD const detour2 = mercator::GetSmPoint(start, 1000.0, 1000.0);
  m2::PointD const startStation = mercator::GetSmPoint(start, 1000.0, 0.0);
  m2::PointD const finishStation = mercator::GetSmPoint(start, 1200.0, 0.0);
  m2::PointD const finish = mercator::GetSmPoint(start, 1400.0, 0.0);
  m2::RectD const countryRect = mercator::RectByCenterXYAndSizeInMeters(start, 4000.0);

  TestStreet street({start, detour1, detour2, startStation, finishStation, finish}, "detour street", "en");

  auto startStationPoi = MakeBicycleRentalStation(startStation, "start-station");
  startStationPoi.GetMetadata().Set(feature::Metadata::FMD_NETWORK, "network");

  auto finishStationPoi = MakeBicycleRentalStation(finishStation, "finish-station");
  finishStationPoi.GetMetadata().Set(feature::Metadata::FMD_NETWORK, "network");

  BuildCountry(kCountryName, [&](TestMwmBuilder & builder)
  {
    builder.Add(street);
    builder.Add(startStationPoi);
    builder.Add(finishStationPoi);
  });
  BuildRoutingSection(kCountryName);

  RoutingOptions options;
  options.Add(RoutingOptions::Road::PublicBicycle);
  BicycleOptionsGuard const optionsGuard(options);

  auto router = MakeBicycleRouter(kCountryName, countryRect);
  RouterDelegate const delegate;
  Route route("public-bicycle-route", 0 /* routeId */);

  auto const result = router.CalculateRoute(Checkpoints(start, finish), m2::PointD::Zero() /* startDirection */,
                                            false /* adjustToPrevRoute */, delegate, route);

  TEST_EQUAL(result, RouterResultCode::RouteNotFound, ());
}
}  // namespace bike_sharing_heuristic_tests
