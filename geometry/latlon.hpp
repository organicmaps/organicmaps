#pragma once

#include "base/math.hpp"

#include <string>

namespace ms
{
/// \brief Class for representing WGS point.
class LatLon
{
public:
  static double const kMinLat;
  static double const kMaxLat;
  static double const kMinLon;
  static double const kMaxLon;

  double lat, lon;

  /// Does NOT initialize lat and lon. Allows to use it as a property of an ObjC class.
  LatLon() = default;
  LatLon(double lat, double lon) : lat(lat), lon(lon) {}

  static LatLon Zero() { return LatLon(0., 0.); }

  bool operator==(ms::LatLon const & p) const;

  bool EqualDxDy(LatLon const & p, double eps) const;

  struct Hash
  {
    size_t operator()(ms::LatLon const & p) const { return my::Hash(p.lat, p.lon); }
  };
};

std::string DebugPrint(LatLon const & t);
}  // namespace ms
