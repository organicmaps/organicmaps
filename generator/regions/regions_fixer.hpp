#pragma once

#include "generator/regions/place_point.hpp"
#include "generator/regions/regions_builder.hpp"

#include "base/geo_object_id.hpp"

#include <vector>

namespace generator
{
namespace regions
{
// This function will build a boundary from point based on place.
void FixRegionsWithPlacePointApproximation(PlacePointsMap const & placePointsMap,
                                           RegionsBuilder::Regions & regions);
}  // namespace regions
}  // namespace generator
