#pragma once
#include "feature.hpp"
#include "../geometry/rect2d.hpp"
#include "../base/base.hpp"

namespace feature
{

// Get viewport to show given feature. Used in search.
m2::RectD GetFeatureViewport(FeatureType const & feature);

// Get search rank for a feature. Roughly, rank + 1 means that feature is 1.x times more popular.
uint8_t GetSearchRank(FeatureType const & feature);

}  // namespace feature
