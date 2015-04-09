#pragma once

#include "geometry/rect2d.hpp"

#include "base/base.hpp"


namespace feature
{
  class TypesHolder;

  /// Get viewport scale to show given feature. Used in search.
  int GetFeatureViewportScale(TypesHolder const & types);

  /// Get search rank for a feature.
  /// Roughly, rank + 1 means that feature is 1.x times more popular.
  uint8_t GetSearchRank(TypesHolder const & types, m2::PointD const & pt, uint32_t population);
}  // namespace feature
