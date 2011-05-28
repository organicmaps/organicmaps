#include "intermediate_result.hpp"
#include "../base/string_utils.hpp"

namespace search
{
namespace impl
{

IntermediateResult::IntermediateResult(FeatureType const & feature,
                                       string const & displayName,
                                       int matchPenalty)
  : m_str(displayName), m_rect(feature.GetLimitRect(-1)), m_matchPenalty(matchPenalty)
{
}

bool IntermediateResult::operator < (IntermediateResult const & o) const
{
  return m_matchPenalty < o.m_matchPenalty;
}

Result IntermediateResult::GenerateFinalResult() const
{
#ifdef DEBUG
  return Result(m_str + ' ' + strings::to_string(m_matchPenalty), m_rect);
#else
  return Result(m_str, m_rect);
#endif
}

}  // namespace search::impl
}  // namespace search
