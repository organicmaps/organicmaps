#include "search/v2/features_layer_matcher.hpp"

#include "search/projection_on_street.hpp"

#include "indexer/scales.hpp"

namespace search
{
namespace v2
{
FeaturesLayerMatcher::FeaturesLayerMatcher(MwmValue & value, FeaturesVector const & featuresVector)
  : m_featuresVector(featuresVector)
  , m_loader(value, featuresVector, scales::GetUpperScale(),
             ProjectionOnStreetCalculator::kDefaultMaxDistMeters)
{
}
}  // namespace v2
}  // namespace search
