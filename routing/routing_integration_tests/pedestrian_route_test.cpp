#include "testing/testing.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "geometry/mercator.hpp"

using namespace routing;
using namespace routing::turns;

UNIT_TEST(GermanyBremenJunctionToCycleway)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(52.41947, 10.75148), {0., 0.},
      MercatorBounds::FromLatLon(52.41868, 10.75274), 137.);
}

UNIT_TEST(Zgrad424aTo1207)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(55.9963, 37.2036), {0., 0.},
      MercatorBounds::FromLatLon(55.9955, 37.1948), 683.);
}

UNIT_TEST(Zgrad924aTo418)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(55.9844, 37.1808), {0., 0.},
      MercatorBounds::FromLatLon(55.9999, 37.2021), 2526.);
}

UNIT_TEST(Zgrad924aToFilaretovskyChurch)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(55.9844, 37.1808), {0., 0.},
      MercatorBounds::FromLatLon(55.9915, 37.1808), 1220.);
}

UNIT_TEST(Zgrad924aTo1145)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(55.9844, 37.1808), {0., 0.},
      MercatorBounds::FromLatLon(55.9924, 37.1853), 1400.);
}

UNIT_TEST(MoscowMuzeonToLebedinoeOzeroGorkyPark)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(55.7348, 37.606), {0., 0.},
      MercatorBounds::FromLatLon(55.724, 37.5956), 1617.);
}

/*
UNIT_TEST(Zgrad315parkingToMusicSchoolBus_BadRoute)
{
  // Bad route:
  // route goes through a highway-tertiary.

  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(55.9996, 37.2174), {0., 0.},
      MercatorBounds::FromLatLon(55.9999, 37.2179), 161.);
}
*/

UNIT_TEST(Zgrad924aToKrukovo)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(55.9844, 37.1808), {0., 0.},
      MercatorBounds::FromLatLon(55.9802, 37.1736), 974.);
}

UNIT_TEST(MoscowMailRuStarbucksToPetrovskoRazumovskyAlley)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(55.7971, 37.5376), {0., 0.},
      MercatorBounds::FromLatLon(55.7953, 37.5597), 1840.);
}

UNIT_TEST(AustraliaMelburn_AvoidMotorway)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(-37.7936, 144.985), {0., 0.},
      MercatorBounds::FromLatLon(-37.7896, 145.025), 5015.);
}

UNIT_TEST(AustriaWein_AvoidTrunk)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(48.233, 16.3562), {0., 0.},
      MercatorBounds::FromLatLon(48.2458, 16.3704), 2301.);
}

UNIT_TEST(FranceParis_AvoidBridleway)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(48.859, 2.25452), {0., 0.},
      MercatorBounds::FromLatLon(48.8634, 2.24315), 1307.);
}

UNIT_TEST(HungaryBudapest_AvoidMotorway)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(47.56566, 19.14942), {0., 0.},
      MercatorBounds::FromLatLon(47.593, 19.24018), 10890.);
}

UNIT_TEST(PolandWarshaw_AvoidCycleway)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(52.2487, 21.0173), {0., 0.},
      MercatorBounds::FromLatLon(52.25, 21.0164), 182.);
}

UNIT_TEST(SwedenStockholmSlussenHiltonToMaritimeMuseum)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(59.32046, 18.06924), {0., 0.},
      MercatorBounds::FromLatLon(59.32728, 18.09078), 3442.);
}

UNIT_TEST(SwedenStockholmSlussenHiltonToAfChapmanHostel)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(59.32045, 18.06928), {0., 0.},
      MercatorBounds::FromLatLon(59.3254, 18.08022), 2410.);
}

UNIT_TEST(EstoniaTallinnRadissonHiltonToCatherdalChurch)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(59.4362, 24.7682), {0., 0.},
      MercatorBounds::FromLatLon(59.437, 24.7392), 2016.);
}

UNIT_TEST(EstoniaTallinnRadissonHiltonToSkypeOffice)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(59.4362, 24.7682), {0., 0.},
      MercatorBounds::FromLatLon(59.3971, 24.661), 8673.);
}

UNIT_TEST(BelarusMinksHotelYubileyniToChurchSaintsSimonAndHelen)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(53.9112, 27.5466), {0., 0.},
      MercatorBounds::FromLatLon(53.8965, 27.5476), 2244.);
}

UNIT_TEST(BelarusMinksBarURatushiToMoscowBusStation)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(53.9045, 27.5569), {0., 0.},
      MercatorBounds::FromLatLon(53.889, 27.5466), 2499.);
}

UNIT_TEST(BelarusBobruisk50LetVlksmToSanatoryShinnik)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(53.1638, 29.1804), {0., 0.},
      MercatorBounds::FromLatLon(53.179, 29.1682), 2661.);
}

UNIT_TEST(BelarusBobruisk50LetVlksmToArena)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(53.1638, 29.1804), {0., 0.},
      MercatorBounds::FromLatLon(53.1424, 29.2467), 6683.);
}

UNIT_TEST(RussiaTaganrogSyzranov10k3ToSoftech)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(47.2183, 38.8634), {0., 0.},
      MercatorBounds::FromLatLon(47.2, 38.8878), 3868.);
}

UNIT_TEST(RussiaTaganrogSyzranov10k3ToTruseE)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(47.2183, 38.8634), {0., 0.},
      MercatorBounds::FromLatLon(47.2048, 38.9441), 7463.);
}

UNIT_TEST(RussiaTaganrogSyzranov10k3ToLazo5k2)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(47.2183, 38.8634), {0., 0.},
      MercatorBounds::FromLatLon(47.2584, 38.9128), 9200.);
}

UNIT_TEST(RussiaTaganrogJukova2ToBolBulvarnaya8)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(47.2768, 38.9282), {0., 0.},
      MercatorBounds::FromLatLon(47.2412, 38.8902), 6239.);
}

UNIT_TEST(RussiaTaganrogCheckhova267k2ToKotlostroy33)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(47.2198, 38.8906), {0., 0.},
      MercatorBounds::FromLatLon(47.2459, 38.8937), 3485.);
}

UNIT_TEST(RussiaTaganrogCheckhova267k2ToBolBulvarnaya8)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(47.2198, 38.8906), {0., 0.},
      MercatorBounds::FromLatLon(47.2412, 38.8902), 2897.);
}

UNIT_TEST(RussiaRostovOnDonPrKosmonavtovToDneprovsky120b)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(47.2811, 39.7178), {0., 0.},
      MercatorBounds::FromLatLon(47.2875, 39.759), 4300.);
}

UNIT_TEST(TurkeyKemerPalmetResortToYachtClub)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(36.6143, 30.5572), {0., 0.},
      MercatorBounds::FromLatLon(36.6004, 30.576), 2992.);
}

UNIT_TEST(CzechPragueNode5ToHilton)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(50.0653, 14.4031), {0., 0.},
      MercatorBounds::FromLatLon(50.0933, 14.4397), 5106.);
}

UNIT_TEST(CzechPragueHiltonToKarlovMost)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(50.0933, 14.4397), {0., 0.},
      MercatorBounds::FromLatLon(50.0864, 14.4124), 2398.);
}

UNIT_TEST(CzechPragueHiltonToNicholasChurch)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(50.0933, 14.4397), {0., 0.},
      MercatorBounds::FromLatLon(50.088, 14.4032), 3103.);
}

UNIT_TEST(CzechPragueHiltonToKvetniceViewpoint)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(50.0933, 14.4397), {0., 0.},
      MercatorBounds::FromLatLon(50.0806, 14.3973), 4335.);
}

UNIT_TEST(RussiaSaintPetersburgMoyka93ToAlexanderColumn)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(59.9241, 30.323), {0., 0.},
      MercatorBounds::FromLatLon(59.939, 30.3159), 2454.);
}

UNIT_TEST(RussiaSaintPetersburgMoyka93ToMarsovoPole)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(59.9241, 30.323), {0., 0.},
      MercatorBounds::FromLatLon(59.9436, 30.3318), 2891.);
}

UNIT_TEST(RussiaSaintPetersburgMoyka93ToAvrora)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(59.9241, 30.323), {0., 0.},
      MercatorBounds::FromLatLon(59.9554, 30.3378), 4770.);
}

UNIT_TEST(RussiaSaintPetersburgPetrPaulChurchToDolphins)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(59.9502, 30.3165), {0., 0.},
      MercatorBounds::FromLatLon(59.973, 30.2702), 4507.);
}

UNIT_TEST(RussiaPetergofEntranceToErmitagePalace)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(59.8806, 29.904), {0., 0.},
      MercatorBounds::FromLatLon(59.8889, 29.9034), 1073.);
}

UNIT_TEST(RussiaPetergofMarlyPalaceToTrainStation)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(59.8887, 29.8963), {0., 0.},
      MercatorBounds::FromLatLon(59.8648, 29.9251), 3885.);
}

UNIT_TEST(RussiaMoscowMailRuToTsarCannon)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(55.79703, 37.53761), {0., 0.},
      MercatorBounds::FromLatLon(55.75146, 37.61792), 7989.);
}

UNIT_TEST(RussiaMoscowHovrinoStationToKasperskyLab)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(55.8701, 37.50833), {0., 0.},
      MercatorBounds::FromLatLon(55.83715, 37.48132), 5162.);
}

UNIT_TEST(ItalyRome_WalkOverStreetWithSidewalkBoth)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(41.9052, 12.4106), {0., 0.},
      MercatorBounds::FromLatLon(41.9226, 12.4216), 2413.);
}

UNIT_TEST(USARedlandsEsriHQToRedlandsCommunity)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(34.0556, -117.19567), {0., 0.},
      MercatorBounds::FromLatLon(34.03682, -117.20649), 3330.);
}

UNIT_TEST(USANewYorkEmpireStateBuildingToUnitedNations)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(40.74844, -73.98566), {0., 0.},
      MercatorBounds::FromLatLon(40.75047, -73.96759), 2265.);
}

UNIT_TEST(CrossMwmEgyptTabaToJordanAqaba)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(29.49271, 34.89571), {0., 0.},
      MercatorBounds::FromLatLon(29.52774, 35.00324), 29016);
}

UNIT_TEST(CrossMwmRussiaPStaiToBelarusDrazdy)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(55.014, 30.95552), {0., 0.},
      MercatorBounds::FromLatLon(55.01437, 30.8858), 4834.5);
}

UNIT_TEST(RussiaZgradPanfilovskyUndergroundCrossing)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Pedestrian>(),
                                  MercatorBounds::FromLatLon(55.98401, 37.17979), {0., 0.},
                                  MercatorBounds::FromLatLon(55.98419, 37.17938));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, IRouter::NoError, ());

  vector<turns::TurnItem> t;
  route.GetTurnsForTesting(t);
  TEST_EQUAL(t.size(), 3, ());

  TEST_EQUAL(t[0].m_pedestrianTurn, PedestrianDirection::Downstairs, ());
  TEST_EQUAL(t[1].m_pedestrianTurn, PedestrianDirection::Upstairs, ());
  TEST_EQUAL(t[2].m_pedestrianTurn, PedestrianDirection::ReachedYourDestination, ());
}

UNIT_TEST(RussiaMoscowHydroprojectBridgeCrossing)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Pedestrian>(),
                                  MercatorBounds::FromLatLon(55.80867, 37.50575), {0., 0.},
                                  MercatorBounds::FromLatLon(55.80884, 37.50668));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, IRouter::NoError, ());

  vector<turns::TurnItem> t;
  route.GetTurnsForTesting(t);
  TEST_EQUAL(t.size(), 3, ());

  TEST_EQUAL(t[0].m_pedestrianTurn, PedestrianDirection::Upstairs, ());
  TEST_EQUAL(t[1].m_pedestrianTurn, PedestrianDirection::Downstairs, ());
  TEST_EQUAL(t[2].m_pedestrianTurn, PedestrianDirection::ReachedYourDestination, ());
}

UNIT_TEST(BelarusMinskRenaissanceHotelUndergroundCross)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Pedestrian>(),
                                  MercatorBounds::FromLatLon(53.89302, 27.52792), {0., 0.},
                                  MercatorBounds::FromLatLon(53.89262, 27.52838));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, IRouter::NoError, ());

  vector<turns::TurnItem> t;
  route.GetTurnsForTesting(t);
  TEST_EQUAL(t.size(), 3, ());

  TEST_EQUAL(t[0].m_pedestrianTurn, PedestrianDirection::Downstairs, ());
  TEST_EQUAL(t[1].m_pedestrianTurn, PedestrianDirection::Upstairs, ());
  TEST_EQUAL(t[2].m_pedestrianTurn, PedestrianDirection::ReachedYourDestination, ());
}

UNIT_TEST(RussiaMoscowTrubnikovPereulok30Ac1LiftGate)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Pedestrian>(),
                                  MercatorBounds::FromLatLon(55.75533, 37.58789), {0., 0.},
                                  MercatorBounds::FromLatLon(55.75543, 37.58717));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, IRouter::NoError, ());

  vector<turns::TurnItem> t;
  route.GetTurnsForTesting(t);
  TEST_EQUAL(t.size(), 2, ());

  TEST_EQUAL(t[0].m_pedestrianTurn, PedestrianDirection::LiftGate, ());
  TEST_EQUAL(t[1].m_pedestrianTurn, PedestrianDirection::ReachedYourDestination, ());
}

UNIT_TEST(RussiaMoscowKhlebnyyLane15c1Gate)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Pedestrian>(),
                                  MercatorBounds::FromLatLon(55.755, 37.59461), {0., 0.},
                                  MercatorBounds::FromLatLon(55.75522, 37.59494));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, IRouter::NoError, ());

  vector<turns::TurnItem> t;
  route.GetTurnsForTesting(t);
  TEST_EQUAL(t.size(), 2, ());

  TEST_EQUAL(t[0].m_pedestrianTurn, PedestrianDirection::Gate, ());
  TEST_EQUAL(t[1].m_pedestrianTurn, PedestrianDirection::ReachedYourDestination, ());
}

UNIT_TEST(RussiaMoscowKhlebnyyLane19LiftGateAndGate)
{
  TRouteResult const routeResult =
      integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Pedestrian>(),
                                  MercatorBounds::FromLatLon(55.75518, 37.59382), {0., 0.},
                                  MercatorBounds::FromLatLon(55.7554, 37.59327));

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, IRouter::NoError, ());

  vector<turns::TurnItem> t;
  route.GetTurnsForTesting(t);
  TEST_EQUAL(t.size(), 3, ());

  TEST_EQUAL(t[0].m_pedestrianTurn, PedestrianDirection::LiftGate, ());
  TEST_EQUAL(t[1].m_pedestrianTurn, PedestrianDirection::Gate, ());
  TEST_EQUAL(t[2].m_pedestrianTurn, PedestrianDirection::ReachedYourDestination, ());
}

UNIT_TEST(MoscowVodnyStadiumHighwayPlatform)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(55.83955, 37.48692), {0., 0.},
      MercatorBounds::FromLatLon(55.84061, 37.48636), 136.115);
}

UNIT_TEST(MoscowChistiePrudiSelectPointsInConnectedGraph)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(55.76613, 37.63769), {0., 0.},
      MercatorBounds::FromLatLon(55.76593, 37.63893), 134.02);
}

UNIT_TEST(RussiaMoscowSevTushinoParkPedestrianOnePointTurnTest)
{
  m2::PointD const point = MercatorBounds::FromLatLon(55.8719, 37.4464);
  TRouteResult const routeResult = integration::CalculateRoute(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(), point, {0.0, 0.0}, point);

  Route const & route = *routeResult.first;
  IRouter::ResultCode const result = routeResult.second;
  TEST_EQUAL(result, IRouter::NoError, ());
  integration::TestTurnCount(route, 0);
  integration::TestRouteLength(route, 0.0);
}

UNIT_TEST(MoscowKashirskoe16ToVorobeviGori)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents<VehicleType::Pedestrian>(),
      MercatorBounds::FromLatLon(55.66230, 37.63214), {0., 0.},
      MercatorBounds::FromLatLon(55.70934, 37.54232), 9553.0);
}
