#pragma once

#include "geometry/point2d.hpp"

#include "std/function.hpp"

class DataSourceBase;
class FeatureType;

namespace indexer
{
void ForEachFeatureAtPoint(DataSourceBase const & dataSource, function<void(FeatureType &)> && fn,
                           m2::PointD const & mercator, double toleranceInMeters = 0.0);
}
