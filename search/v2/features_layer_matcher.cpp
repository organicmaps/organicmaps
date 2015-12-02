#include "search/v2/features_layer_matcher.hpp"

namespace search
{
namespace v2
{
namespace
{
static double constexpr kDefaultRadiusMeters = 200;
}  // namespace

FeaturesLayerMatcher::FeaturesLayerMatcher(MwmValue & value, FeaturesVector const & featuresVector)
  : m_featuresVector(featuresVector), m_loader(value, featuresVector, kDefaultRadiusMeters)
{
}
}  // namespace v2
}  // namespace search
