#pragma once

#include "geometry/region2d.hpp"

#include <vector>

namespace m2
{
void IntersectRegions(RegionI const & r1, RegionI const & r2, std::vector<RegionI> & res);
void DiffRegions(RegionI const & r1, RegionI const & r2, std::vector<RegionI> & res);
}  // namespace m2
