#pragma once

#include "geometry/point2d.hpp"

#include <functional>

class DataSource;
class FeatureType;

namespace indexer
{
void ForEachFeatureAtPoint(DataSource const & dataSource, std::function<void(FeatureType &)> && fn,
                           m2::PointD const & mercator, double toleranceInMeters = 0.0);
}
