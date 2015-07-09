#pragma once

#include "std/string.hpp"

namespace ms
{

/// \brief Class for representing WGS point.
class LatLon
{
public:
  LatLon(double lat, double lon) : lat(lat), lon(lon) {}
  double lat, lon;
};

string DebugPrint(LatLon const & t);

}  // namespace ms
