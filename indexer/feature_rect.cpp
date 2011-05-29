#include "feature_rect.hpp"

m2::RectD feature::GetFeatureViewport(FeatureType const & feature)
{
  return feature.GetLimitRect(-1);
}
