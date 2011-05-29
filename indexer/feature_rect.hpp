#pragma once
#include "feature.hpp"
#include "../geometry/rect2d.hpp"

namespace feature
{

// Get viewport to show given feature. Used in search.
m2::RectD GetFeatureViewport(FeatureType const & feature);

}  // namespace feature
