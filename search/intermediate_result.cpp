#include "intermediate_result.hpp"
#include "../indexer/feature_rect.hpp"
#include "../base/string_utils.hpp"

namespace search
{
namespace impl
{

IntermediateResult::IntermediateResult(FeatureType const & feature,
                                       string const & displayName,
                                       int matchPenalty,
                                       int minVisibleScale)
  : m_str(displayName), m_rect(feature::GetFeatureViewport(feature)), m_matchPenalty(matchPenalty),
    m_minVisibleScale(minVisibleScale)
{
  FeatureType::GetTypesFn types;
  feature.ForEachTypeRef(types);
  ASSERT_GREATER(types.m_size, 0, ());
  m_type = types.m_types[0];
}

bool IntermediateResult::operator < (IntermediateResult const & o) const
{
  if (m_matchPenalty != o.m_matchPenalty)
    return m_matchPenalty < o.m_matchPenalty;
  if (m_minVisibleScale != o.m_minVisibleScale)
    return m_minVisibleScale < o.m_minVisibleScale;
  return false;
}

Result IntermediateResult::GenerateFinalResult() const
{
#ifdef DEBUG
  return Result(m_str
                + ' ' + strings::to_string(m_matchPenalty)
                + ' ' + strings::to_string(m_minVisibleScale),
#else
  return Result(m_str,
#endif
                m_type, m_rect);
}

}  // namespace search::impl
}  // namespace search
