#pragma once

#include "geometry/latlon.hpp"

#include "indexer/feature_altitude.hpp"

#include "std/string.hpp"

namespace routing
{
class IAltitudeGetter
{
public:
  virtual feature::TAltitude GetAltitude(ms::LatLon const & coord) = 0;
};

void BuildRoadAltitudes(IAltitudeGetter const & altitudeGetter, string const & baseDir,
                        string const & countryName);
void BuildRoadAltitudes(string const & srtmPath, string const & baseDir,
                        string const & countryName);
}  // namespace routing
