#pragma once

#include "routing_common/maxspeed_conversion.hpp"
#include "routing_common/vehicle_model.hpp"

// These are default car model coefficients for open source developers.

namespace routing
{
HighwayBasedFactors const kGlobalHighwayBasedFactors = {
  {HighwayType::HighwayMotorway, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(1.0)}
  }},
  {HighwayType::HighwayTrunk, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(1.0)}
  }},
  {HighwayType::HighwayPrimary /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.95)}
  }},
  {HighwayType::HighwaySecondary /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.90)}
  }},
  {HighwayType::HighwayTertiary /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.85)}
  }},
  {HighwayType::HighwayResidential /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.75)}
  }},
  {HighwayType::HighwayUnclassified /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.80)}
  }},
  {HighwayType::HighwayService /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.80)}
  }},
  {HighwayType::HighwayLiving_street /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.75)}
  }},
  {HighwayType::HighwayRoad /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.30)}
  }},
  {HighwayType::HighwayTrack /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.30)}
  }},
  {HighwayType::HighwayTrack /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.30)}
  }},
  {HighwayType::RouteFerryMotorcar /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.90)}
  }},
  {HighwayType::RouteFerryMotorVehicle /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.90)}
  }},
  {HighwayType::RailwayRailMotorVehicle /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.90)}
  }},
  {HighwayType::RouteShuttleTrain /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.90)}
  }},
  {HighwayType::ManMadePier/* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.90)}
  }},
};

HighwayBasedMeanSpeeds const kGlobalHighwayBasedMeanSpeeds = {
  // {highway class : InOutCitySpeedKMpH(in city, out city)}
  {HighwayType::HighwayMotorway, InOutCitySpeedKMpH({117.80, 104.70} /* in city */, {123.40, 111.79} /* out city */)},
  {HighwayType::HighwayMotorwayLink, InOutCitySpeedKMpH({106.02, 94.23} /* in city */, {111.06, 100.61} /* out city */)},
  {HighwayType::HighwayTrunk, InOutCitySpeedKMpH({83.40, 78.55} /* in city */, {100.20, 92.55} /* out city */)},
  {HighwayType::HighwayTrunkLink, InOutCitySpeedKMpH({75.06, 70.69} /* in city */, {90.18, 83.30} /* out city */)},
  {HighwayType::HighwayPrimary, InOutCitySpeedKMpH({63.10, 58.81} /* in city */, {75.20, 69.60} /* out city */)},
  {HighwayType::HighwayPrimaryLink, InOutCitySpeedKMpH({56.79, 52.93} /* in city */, {67.68, 62.64} /* out city */)},
  {HighwayType::HighwaySecondary, InOutCitySpeedKMpH({52.80, 47.63} /* in city */, {60.30, 56.99} /* out city */)},
  {HighwayType::HighwaySecondaryLink, InOutCitySpeedKMpH({47.52, 42.87} /* in city */, {54.27, 51.29} /* out city */)},
  {HighwayType::HighwayTertiary, InOutCitySpeedKMpH({45.50, 38.86} /* in city */, {50.50, 44.14} /* out city */)},
  {HighwayType::HighwayTertiaryLink, InOutCitySpeedKMpH({40.95, 34.97} /* in city */, {45.45, 39.73} /* out city */)},
  {HighwayType::HighwayResidential, InOutCitySpeedKMpH({20.00, 20.00} /* in city */, {25.00, 25.00} /* out city */)},
  {HighwayType::HighwayUnclassified, InOutCitySpeedKMpH({51.30, 51.30} /* in city */, {66.00, 66.00} /* out city */)},
  {HighwayType::HighwayService, InOutCitySpeedKMpH({15.00, 15.00} /* in city */, {15.00, 15.00} /* out city */)},
  {HighwayType::HighwayLivingStreet, InOutCitySpeedKMpH({10.00, 10.00} /* in city */, {10.00, 10.00} /* out city */)},
  {HighwayType::HighwayRoad, InOutCitySpeedKMpH({10.00, 10.00} /* in city */, {10.00, 10.00} /* out city */)},
  {HighwayType::HighwayTrack, InOutCitySpeedKMpH({5.00, 5.00} /* in city */, {5.00, 5.00} /* out city */)},
  {HighwayType::RouteFerryMotorcar, InOutCitySpeedKMpH({10.00, 10.00} /* in city */, {10.00, 10.00} /* out city */)},
  {HighwayType::RouteFerryMotorVehicle, InOutCitySpeedKMpH({10.00, 10.00} /* in city */, {10.00, 10.00} /* out city */)},
  {HighwayType::RailwayRailMotorVehicle, InOutCitySpeedKMpH({10.00, 10.00} /* in city */, {10.00, 10.00} /* out city */)},
  {HighwayType::RouteShuttleTrain, InOutCitySpeedKMpH({25.00, 25.00} /* in city */, {25.00, 25.00} /* out city */)},
  {HighwayType::ManMadePier, InOutCitySpeedKMpH({17.00, 10.00} /* in city */, {17.00, 10.00} /* out city */)},
};

CountryToHighwayBasedFactors const kCountryToHighwayBasedFactors{};
CountryToHighwayBasedMeanSpeeds const kCountryToHighwayBasedMeanSpeeds{};
}  // namespace routing
