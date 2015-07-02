#pragma once

#include "indexer/mercator.hpp"

#include "std/sstream.hpp"
#include "std/string.hpp"

namespace ms
{
/// \brief Class for representing WGS point.
class LatLon
{
public:
  LatLon(m2::PointD point)
      : lat(MercatorBounds::YToLat(point.y)), lon(MercatorBounds::XToLon(point.x))
  {
  }

  double lat, lon;
};

inline string DebugPrint(LatLon const & t)
{
  ostringstream out;
  out << "LatLon [ " << t.lat << ", " << t.lon << " ]";
  return out.str();
}

}  // namespace ms
