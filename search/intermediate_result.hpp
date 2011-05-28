#pragma once
#include "result.hpp"
#include "../indexer/feature.hpp"

namespace search
{
namespace impl
{

class IntermediateResult
{
public:
  IntermediateResult(FeatureType const & feature, string const & displayName, int matchPenalty);

  bool operator < (IntermediateResult const & o) const;

  Result GenerateFinalResult() const;

private:
  string m_str;
  m2::RectD m_rect;
  int m_matchPenalty;
  int m_minDrawZoomLevel;
};

}  // namespace search::impl
}  // namespace search
