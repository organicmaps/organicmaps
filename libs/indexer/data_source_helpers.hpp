#pragma once

#include "indexer/scales.hpp"

#include "geometry/point2d.hpp"

#include <functional>

class DataSource;
class FeatureType;

namespace indexer
{
/// @param[in] scale Consider keep GetUpperScale (for countries), or pass GetUpperWorldScale (for World map).
void ForEachFeatureAtPoint(DataSource const & dataSource, std::function<void(FeatureType &)> && fn,
                           m2::PointD const & mercator, int scale = scales::GetUpperScale());
}  // namespace indexer
