#pragma once

#include "geometry/point2d.hpp"

#include "std/function.hpp"

class Index;
class FeatureType;

namespace indexer
{
void ForEachFeatureAtPoint(Index const & index, function<void(FeatureType &)> && fn,
                           m2::PointD const & mercator, double toleranceInMeters = 0.0);
}
