#pragma once

#include "geometry/point2d.hpp"

#include "indexer/feature_altitude.hpp"

#include "std/string.hpp"

namespace routing
{
class IAltitudeGetter
{
public:
  virtual feature::TAltitude GetAltitude(m2::PointD const & p) = 0;
};

void BuildRoadAltitudes(string const & baseDir, string const & countryName,
                        IAltitudeGetter & altitudeGetter);
void BuildRoadAltitudes(string const & srtmPath, string const & baseDir,
                        string const & countryName);
}  // namespace routing
