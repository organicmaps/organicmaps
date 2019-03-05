#pragma once

#include "routing_common/maxspeed_conversion.hpp"
#include "routing_common/vehicle_model.hpp"

// These are default car model coefficients for open source developers.

namespace routing
{
HighwayBasedFactors const kGlobalHighwayBasedFactors = {
  {"highway-motorway" /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(1.0)}
  }},
  {"highway-trunk" /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(1.0)}
  }},
  {"highway-primary" /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.95)}
  }},
  {"highway-secondary" /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.90)}
  }},
  {"highway-tertiary" /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.85)}
  }},
  {"highway-residential" /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.75)}
  }},
  {"highway-unclassified" /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.80)}
  }},
  {"highway-service" /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.80)}
  }},
  {"highway-living_street" /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.75)}
  }},
  {"highway-road" /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.30)}
  }},
  {"highway-track" /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.30)}
  }},
  {"highway-track" /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.30)}
  }},
  {"highway-ferry-motorcar" /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.90)}
  }},
  {"highway-ferry-motor_vehicle" /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.90)}
  }},
  {"highway-rail-motor_vehicle" /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.90)}
  }},
  {"highway-shuttle_train" /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.90)}
  }},
  {"man_made-pier" /* highway class */, {
    // {maxspeed : InOutCityFactor(in city, out city)}
    {kCommonMaxSpeedValue, InOutCityFactor(0.90)}
  }},
};

HighwayBasedMeanSpeeds const kGlobalHighwayBasedMeanSpeeds = {
  // {highway class : InOutCitySpeedKMpH(in city, out city)}
  {"highway-motorway", InOutCitySpeedKMpH({117.80, 104.70} /* in city */, {123.40, 111.79} /* out city */)},
  {"highway-motorway_link", InOutCitySpeedKMpH({106.02, 94.23} /* in city */, {111.06, 100.61} /* out city */)},
  {"highway-trunk", InOutCitySpeedKMpH({83.40, 78.55} /* in city */, {100.20, 92.55} /* out city */)},
  {"highway-trunk_link", InOutCitySpeedKMpH({75.06, 70.69} /* in city */, {90.18, 83.30} /* out city */)},
  {"highway-primary", InOutCitySpeedKMpH({63.10, 58.81} /* in city */, {75.20, 69.60} /* out city */)},
  {"highway-primary_link", InOutCitySpeedKMpH({56.79, 52.93} /* in city */, {67.68, 62.64} /* out city */)},
  {"highway-secondary", InOutCitySpeedKMpH({52.80, 47.63} /* in city */, {60.30, 56.99} /* out city */)},
  {"highway-secondary_link", InOutCitySpeedKMpH({47.52, 42.87} /* in city */, {54.27, 51.29} /* out city */)},
  {"highway-tertiary", InOutCitySpeedKMpH({45.50, 38.86} /* in city */, {50.50, 44.14} /* out city */)},
  {"highway-tertiary_link", InOutCitySpeedKMpH({40.95, 34.97} /* in city */, {45.45, 39.73} /* out city */)},
  {"highway-residential", InOutCitySpeedKMpH({20.00, 20.00} /* in city */, {25.00, 25.00} /* out city */)},
  {"highway-unclassified", InOutCitySpeedKMpH({51.30, 51.30} /* in city */, {66.00, 66.00} /* out city */)},
  {"highway-service", InOutCitySpeedKMpH({15.00, 15.00} /* in city */, {15.00, 15.00} /* out city */)},
  {"highway-living_street", InOutCitySpeedKMpH({10.00, 10.00} /* in city */, {10.00, 10.00} /* out city */)},
  {"highway-road", InOutCitySpeedKMpH({10.00, 10.00} /* in city */, {10.00, 10.00} /* out city */)},
  {"highway-track", InOutCitySpeedKMpH({5.00, 5.00} /* in city */, {5.00, 5.00} /* out city */)},
  {"route-ferry-motorcar", InOutCitySpeedKMpH({10.00, 10.00} /* in city */, {10.00, 10.00} /* out city */)},
  {"route-ferry-motor_vehicle", InOutCitySpeedKMpH({10.00, 10.00} /* in city */, {10.00, 10.00} /* out city */)},
  {"railway-rail-motor_vehicle", InOutCitySpeedKMpH({10.00, 10.00} /* in city */, {10.00, 10.00} /* out city */)},
  {"route-shuttle_train", InOutCitySpeedKMpH({25.00, 25.00} /* in city */, {25.00, 25.00} /* out city */)},
  {"man_made-pier", InOutCitySpeedKMpH({17.00, 10.00} /* in city */, {17.00, 10.00} /* out city */)},
};

CountryToHighwayBasedFactors const kCountryToHighwayBasedFactors{};
CountryToHighwayBasedMeanSpeeds const kCountryToHighwayBasedMeanSpeeds{};
}  // namespace routing
