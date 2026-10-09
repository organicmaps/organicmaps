#pragma once

#include "geometry/region2d.hpp"

#include <vector>

namespace borders
{
// Matching source arcs share simplification decisions; junctions and repeated
// vertices stay fixed. A newly invalid ring locks its arcs in all their owners.
std::vector<m2::RegionD> SimplifyBorders(std::vector<m2::RegionD> const & borders, double squareEpsilon);
}  // namespace borders
