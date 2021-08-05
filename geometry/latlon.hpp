#pragma once

#include "base/math.hpp"

#include <string>

namespace ms
{
/// \brief Class for representing WGS point.
class LatLon
{
public:
  static double constexpr kMinLat = -90.0;
  static double constexpr kMaxLat = 90.0;
  static double constexpr kMinLon = -180.0;
  static double constexpr kMaxLon = 180.0;

  // Default values are invalid.
  double m_lat = kMinLon;
  double m_lon = kMinLon;

  LatLon() = default;
  LatLon(double lat, double lon) : m_lat(lat), m_lon(lon) {}

  static LatLon Zero() { return LatLon(0.0, 0.0); }

  bool operator==(ms::LatLon const & rhs) const;
  bool operator<(ms::LatLon const & rhs) const;

  bool EqualDxDy(LatLon const & p, double eps) const;

  struct Hash
  {
    size_t operator()(ms::LatLon const & p) const { return base::Hash(p.m_lat, p.m_lon); }
  };
};

std::string DebugPrint(LatLon const & t);
}  // namespace ms

namespace base
{
bool AlmostEqualAbs(ms::LatLon const & ll1, ms::LatLon const & ll2, double const & eps);
}  // namespace base
