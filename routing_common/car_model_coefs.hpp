#pragma once

#include "routing_common/maxspeed_conversion.hpp"
#include "routing_common/vehicle_model.hpp"

// These are default car model coefficients for open source developers.

namespace routing
{
HighwayBasedFactors const kHighwayBasedFactors = {
    // {highway class : InOutCityFactor(in city, out city)}
    {HighwayType::HighwayLivingStreet, InOutCityFactor(0.75)},
    {HighwayType::HighwayMotorway, InOutCityFactor(0.90, 0.94)},
    {HighwayType::HighwayMotorwayLink, InOutCityFactor(0.90, 0.94)},
    {HighwayType::HighwayPrimary, InOutCityFactor(0.86, 0.89)},
    {HighwayType::HighwayPrimaryLink, InOutCityFactor(0.86, 0.89)},
    {HighwayType::HighwayResidential, InOutCityFactor(0.75)},
    {HighwayType::HighwayRoad, InOutCityFactor(0.30)},
    {HighwayType::HighwaySecondary, InOutCityFactor(0.84, 0.82)},
    {HighwayType::HighwaySecondaryLink, InOutCityFactor(0.84, 0.82)},
    {HighwayType::HighwayService, InOutCityFactor(0.80)},
    {HighwayType::HighwayTertiary, InOutCityFactor(0.82, 0.76)},
    {HighwayType::HighwayTertiaryLink, InOutCityFactor(0.82, 0.76)},
    {HighwayType::HighwayTrack, InOutCityFactor(0.30)},
    {HighwayType::HighwayTrack, InOutCityFactor(0.30)},
    {HighwayType::HighwayTrunk, InOutCityFactor(0.90, 0.91)},
    {HighwayType::HighwayTrunkLink, InOutCityFactor(0.77, 0.81)},
    {HighwayType::HighwayUnclassified, InOutCityFactor(0.80)},
    {HighwayType::ManMadePier, InOutCityFactor(0.90)},
    {HighwayType::RailwayRailMotorVehicle, InOutCityFactor(0.90)},
    {HighwayType::RouteFerryMotorcar, InOutCityFactor(0.90)},
    {HighwayType::RouteFerryMotorVehicle, InOutCityFactor(0.90)},
    {HighwayType::RouteShuttleTrain, InOutCityFactor(0.90)},
};

HighwayBasedSpeeds const kHighwayBasedSpeeds = {
    // {highway class : InOutCitySpeedKMpH(in city, out city)}
    {HighwayType::HighwayLivingStreet, InOutCitySpeedKMpH({10.00, 10.00} /* in city */, {10.00, 10.00} /* out city */)},
    {HighwayType::HighwayMotorway, InOutCitySpeedKMpH(118.0 /* in city */, 124.0 /* out city */)},
    {HighwayType::HighwayMotorwayLink, InOutCitySpeedKMpH(109.00 /* in city */, 115.00 /* out city */)},
    {HighwayType::HighwayPrimary, InOutCitySpeedKMpH(65.00 /* in city */, 82.00 /* out city */)},
    {HighwayType::HighwayPrimaryLink, InOutCitySpeedKMpH(58.00 /* in city */, 72.00 /* out city */)},
    {HighwayType::HighwayResidential, InOutCitySpeedKMpH({20.00, 26.00} /* in city */, {26.00, 26.00} /* out city */)},
    {HighwayType::HighwayRoad, InOutCitySpeedKMpH({10.00, 10.00} /* in city */, {10.00, 10.00} /* out city */)},
    {HighwayType::HighwaySecondary, InOutCitySpeedKMpH(60.00 /* in city */, 70.00 /* out city */)},
    {HighwayType::HighwaySecondaryLink, InOutCitySpeedKMpH(48.00 /* in city */, 56.00 /* out city */)},
    {HighwayType::HighwayService, InOutCitySpeedKMpH({15.00, 15.00} /* in city */, {15.00, 15.00} /* out city */)},
    {HighwayType::HighwayTertiary, InOutCitySpeedKMpH(60.00 /* in city */, 50.00 /* out city */)},
    {HighwayType::HighwayTertiaryLink, InOutCitySpeedKMpH({40.95, 34.97} /* in city */, {45.45, 39.73} /* out city */)},
    {HighwayType::HighwayTrack, InOutCitySpeedKMpH({5.00, 5.00} /* in city */, {5.00, 5.00} /* out city */)},
    {HighwayType::HighwayTrunk, InOutCitySpeedKMpH(90.00 /* in city */, 103.00 /* out city */)},
    {HighwayType::HighwayTrunkLink, InOutCitySpeedKMpH(77.00 /* in city */, 91.00 /* out city */)},
    {HighwayType::HighwayUnclassified, InOutCitySpeedKMpH({30.00, 30.00} /* in city */, {40.00, 40.00} /* out city */)},
    {HighwayType::ManMadePier, InOutCitySpeedKMpH({17.00, 10.00} /* in city */, {17.00, 10.00} /* out city */)},
    {HighwayType::RailwayRailMotorVehicle, InOutCitySpeedKMpH({10.00, 10.00} /* in city */, {10.00, 10.00} /* out city */)},
    {HighwayType::RouteFerryMotorcar, InOutCitySpeedKMpH({10.00, 10.00} /* in city */, {10.00, 10.00} /* out city */)},
    {HighwayType::RouteFerryMotorVehicle, InOutCitySpeedKMpH({10.00, 10.00} /* in city */, {10.00, 10.00} /* out city */)},
    {HighwayType::RouteShuttleTrain, InOutCitySpeedKMpH({25.00, 25.00} /* in city */, {25.00, 25.00} /* out city */)},
};
}  // namespace routing
