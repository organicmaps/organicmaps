#include "intermediate_result.hpp"
#include "../indexer/feature_rect.hpp"
#include "../indexer/feature_visibility.hpp"
#include "../base/string_utils.hpp"

namespace search
{
namespace impl
{

IntermediateResult::IntermediateResult(FeatureType const & feature,
                                       string const & displayName,
                                       int matchPenalty)
  : m_str(displayName), m_rect(feature::GetFeatureViewport(feature)), m_matchPenalty(matchPenalty),
    m_minDrawZoomLevel(feature::MinDrawableScaleForFeature(feature))
{
}

bool IntermediateResult::operator < (IntermediateResult const & o) const
{
  if (m_matchPenalty != o.m_matchPenalty)
    return m_matchPenalty < o.m_matchPenalty;
  if (m_minDrawZoomLevel != o.m_minDrawZoomLevel)
    return m_minDrawZoomLevel < o.m_minDrawZoomLevel;
  return false;
}

Result IntermediateResult::GenerateFinalResult() const
{
#ifdef DEBUG
  return Result(m_str
                + ' ' + strings::to_string(m_matchPenalty)
                + ' ' + strings::to_string(m_minDrawZoomLevel),
                m_rect);
#else
  return Result(m_str, m_rect);
#endif
}

}  // namespace search::impl
}  // namespace search
