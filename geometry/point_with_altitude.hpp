#pragma once

#include "geometry/point2d.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace geometry
{
using Altitude = int16_t;
using Altitudes = std::vector<Altitude>;

Altitude constexpr kInvalidAltitude = std::numeric_limits<Altitude>::min();
Altitude constexpr kDefaultAltitudeMeters = 0;

class PointWithAltitude
{
public:
  PointWithAltitude();
  PointWithAltitude(m2::PointD const & point, Altitude altitude);
  PointWithAltitude(PointWithAltitude const &) = default;
  PointWithAltitude & operator=(PointWithAltitude const &) = default;

  bool operator==(PointWithAltitude const & r) const { return m_point == r.m_point; }
  bool operator!=(PointWithAltitude const & r) const { return !(*this == r); }
  bool operator<(PointWithAltitude const & r) const { return m_point < r.m_point; }

  m2::PointD const & GetPoint() const { return m_point; }
  Altitude GetAltitude() const { return m_altitude; }

private:
  friend std::string DebugPrint(PointWithAltitude const & r);

  m2::PointD m_point;
  Altitude m_altitude;
};

std::string DebugPrint(PointWithAltitude const & r);

template <typename T>
m2::Point<T> GetPoint(m2::Point<T> const & point) { return point; }
inline m2::PointD GetPoint(PointWithAltitude const & pwa) { return pwa.GetPoint(); }

inline PointWithAltitude MakePointWithAltitudeForTesting(m2::PointD const & point)
{
  return PointWithAltitude(point, kDefaultAltitudeMeters);
}

bool AlmostEqualAbs(PointWithAltitude const & lhs, PointWithAltitude const & rhs, double eps);
}  // namespace geometry

namespace std
{
template <>
struct hash<geometry::PointWithAltitude>
{
  size_t operator()(geometry::PointWithAltitude const & point) const;
};
}  // namespace std
