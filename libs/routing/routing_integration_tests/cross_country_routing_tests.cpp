#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "testing/testing.hpp"

#include "geometry/mercator.hpp"

using namespace routing;

// In this case the shortest way from Austria, Morzg to Austria, Unken is through Germany. We don't
// add penalty for crossing the Schengen Area borders so the route runs through Germany.
UNIT_TEST(CrossCountry_Schengen_Borders_Austria_to_Austria_through_Germany)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents(VehicleType::Car),
      mercator::FromLatLon(47.7707543, 13.0557409) /* startPoint */, {0.0, 0.0} /* startDirection */,
      mercator::FromLatLon(47.6500734, 12.7291784) /* finalPoint */, 41126.6 /* expectedRouteMeters */);
}

// In this case the shortest way from Russian Federation, Smolensk Oblast to Russian Federation,
// Bryansk Oblast is through Belarus. We don't add penalty for crossing the Eurasian Economic Union
// borders so the route runs through Belarus.
UNIT_TEST(CrossCountry_EAEU_Borders_Russia_to_Russia_through_Belarus)
{
  /// @todo Uses tertiary instead of longer (7km) primary. Can't say fo sure is it ok or not, but looks good.
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents(VehicleType::Car), mercator::FromLatLon(53.83281, 32.47602) /* startPoint */,
      {0.0, 0.0} /* startDirection */, mercator::FromLatLon(52.79681, 32.98167) /* finalPoint */,
      188'085 /* expectedRouteMeters */);
}

UNIT_TEST(Russia_UsePrimaryDetour_NotSecondaryTown)
{
  integration::CalculateRouteAndTestRouteLength(integration::GetVehicleComponents(VehicleType::Car),
                                                mercator::FromLatLon(50.215143, 39.4498585), {0.0, 0.0},
                                                mercator::FromLatLon(49.9025281, 40.5213712), 123949);

  /// @todo Should prefer trunk detour with maxspeed=90 than secondary with maxspeed=60.
  integration::CalculateRouteAndTestRouteLength(integration::GetVehicleComponents(VehicleType::Car),
                                                mercator::FromLatLon(45.2849983, 38.2432247), {0.0, 0.0},
                                                mercator::FromLatLon(45.237346, 38.0046459), 28534.8);
}

// In this case the shortest way from Belgorod oblast to Crimea is through Ukraine. But we add
// |kCrossCountryPenaltyS| penalty for crossing borders between Russian Federation and Ukraine,
// between Ukraine and Crimea, and between Crimea and Russian Federation, because Crimea is a
// territorial dispute. So the route should run directly from Russian Federation to Crimea.
UNIT_TEST(CrossCountry_Russia_Belgorod_Oblast_to_Crimea)
{
  // This routes go via Russia only.
  // Some subroutes are checked in Russia_UsePrimaryDetour_NotSecondaryTown.
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents(VehicleType::Car), mercator::FromLatLon(50.39589, 38.83377) /* startPoint */,
      {0.0, 0.0} /* startDirection */, mercator::FromLatLon(45.06336, 34.48566) /* finalPoint */, 1'144'970);

  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents(VehicleType::Car), mercator::FromLatLon(50.39589, 38.83377) /* startPoint */,
      {0.0, 0.0} /* startDirection */, mercator::FromLatLon(45.1048391, 35.1297058) /* finalPoint */, 1'096'360);
}

// In this case the shortest way from Lithuania to Poland is through Russia, Kaliningrad Oblast. But
// we add cross-country penalty for entering Kaliningrad and don't add it for crossing mutual
// borders of the Schengen Area countries. So the route should run directly from Lithuania to Poland.
UNIT_TEST(CrossCountry_Lithuania_to_Poland)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents(VehicleType::Car), mercator::FromLatLon(55.10055, 22.30228) /* startPoint */,
      {0.0, 0.0} /* startDirection */, mercator::FromLatLon(54.27745, 22.33767) /* finalPoint */,
      191'963 /* expectedRouteMeters */);
}

// In this case the shortest way from Hungary to Slovakia is through Ukraine. But we add penalty for
// crossing cross-country borders if both countries are not in the Schengen Area or Eurasian
// Economic Union. So the route should run directly from Hungary to Slovakia.
UNIT_TEST(CrossCountry_Hungary_to_Slovakia)
{
  integration::CalculateRouteAndTestRouteLength(
      integration::GetVehicleComponents(VehicleType::Car), mercator::FromLatLon(48.39107, 22.18352) /* startPoint */,
      {0.0, 0.0} /* startDirection */, mercator::FromLatLon(48.69826, 22.23454) /* finalPoint */,
      100'015 /* expectedRouteMeters */);
}
