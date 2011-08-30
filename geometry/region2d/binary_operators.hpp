#pragma once

#include "../region2d.hpp"

#include "../../std/vector.hpp"


namespace m2
{
  void IntersectRegions(RegionI const & r1, RegionI const & r2, vector<RegionI> & res);
}
