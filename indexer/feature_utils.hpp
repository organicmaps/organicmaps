#pragma once

#include "geometry/rect2d.hpp"

#include "base/base.hpp"


namespace feature
{
  class TypesHolder;

  /// Get viewport scale to show given feature. Used in search.
  int GetFeatureViewportScale(TypesHolder const & types);
}  // namespace feature
