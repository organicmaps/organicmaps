#pragma once

#include "geometry/point2d.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace geometry
{
using TAltitude = int16_t;
using TAltitudes = std::vector<TAltitude>;

TAltitude constexpr kInvalidAltitude = std::numeric_limits<TAltitude>::min();
TAltitude constexpr kDefaultAltitudeMeters = 0;

double constexpr kPointsEqualEpsilon = 1e-6;

class PointWithAltitude
{
public:
  PointWithAltitude();
  PointWithAltitude(m2::PointD const & point, TAltitude altitude);
  PointWithAltitude(PointWithAltitude const &) = default;
  PointWithAltitude & operator=(PointWithAltitude const &) = default;

  bool operator==(PointWithAltitude const & r) const { return m_point == r.m_point; }
  bool operator!=(PointWithAltitude const & r) const { return !(*this == r); }
  bool operator<(PointWithAltitude const & r) const { return m_point < r.m_point; }

  m2::PointD const & GetPoint() const { return m_point; }
  TAltitude GetAltitude() const { return m_altitude; }

private:
  friend std::string DebugPrint(PointWithAltitude const & r);

  m2::PointD m_point;
  TAltitude m_altitude;
};

std::string DebugPrint(PointWithAltitude const & r);

inline PointWithAltitude MakePointWithAltitudeForTesting(m2::PointD const & point)
{
  return PointWithAltitude(point, kDefaultAltitudeMeters);
}

inline bool AlmostEqualAbs(PointWithAltitude const & lhs, PointWithAltitude const & rhs)
{
  return base::AlmostEqualAbs(lhs.GetPoint(), rhs.GetPoint(), kPointsEqualEpsilon);
}
}  // namespace geometry
