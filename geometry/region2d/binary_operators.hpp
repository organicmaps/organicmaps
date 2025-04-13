#pragma once

#include "geometry/region2d.hpp"

#include <vector>

namespace m2
{
using MultiRegionI = std::vector<RegionI>;

/// @name Next functions work in _append_ mode to the result.
/// @{
void IntersectRegions(RegionI const & r1, RegionI const & r2, MultiRegionI & res);
void DiffRegions(RegionI const & r1, RegionI const & r2, MultiRegionI & res);
/// @}

MultiRegionI IntersectRegions(RegionI const & r1, MultiRegionI const & r2);

/// Union \a r with \a res and save to \a res.
void AddRegion(RegionI const & r, MultiRegionI & res);

uint64_t Area(MultiRegionI const & rgn);
}  // namespace m2
