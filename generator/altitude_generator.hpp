#pragma once

#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"

#include "indexer/feature_altitude.hpp"

#include <string>

namespace routing
{
class AltitudeGetter
{
public:
  virtual geometry::Altitude GetAltitude(m2::PointD const & p) = 0;
};

/// \brief Adds altitude section to mwm. It has the following format:
/// File offset (bytes) Field name            Field size (bytes)
/// 0                   version               2
/// 2                   min altitude          2
/// 4                   feature table offset  4
/// 8                   altitude info offset  4
/// 12                  end of section        4
/// 16                  altitude availability feat. table offset - 16
/// feat. table offset  feature table         alt. info offset - feat. table offset
/// alt. info offset    altitude info         end of section - alt. info offset
void BuildRoadAltitudes(std::string const & mwmPath, AltitudeGetter & altitudeGetter);
void BuildRoadAltitudes(std::string const & mwmPath, std::string const & srtmDir);
}  // namespace routing
