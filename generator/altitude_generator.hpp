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

/// \brief Adds altitude section to mwm. It has the following format:
/// File offset (bytes) Field name            Field size (bytes)
/// 0                   version               2
/// 2                   min altitude          2
/// 4                   feature table offset  4
/// 8                   altitude info offset  4
/// 12                  end of section        4
/// 16                  altitude availability feat. table offset - 16
/// feat. table offset  feature table         alt. info offset - f. table offset
/// alt. info offset    altitude info         alt. info offset - end of section
void BuildRoadAltitudes(string const & mwmPath, IAltitudeGetter & altitudeGetter);
void BuildRoadAltitudes(string const & mwmPath, string const & srtmPath);
}  // namespace routing
