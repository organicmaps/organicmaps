#pragma once

#include "geometry/region2d.hpp"
#include "geometry/point2d.hpp"

#include "std/vector.hpp"
#include "std/string.hpp"

namespace osm
{
  /// @return false if borderFile can't be opened
  bool LoadBorders(string const & borderFile, vector<m2::RegionD> & outBorders);
}
