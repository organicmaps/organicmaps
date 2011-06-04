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
  IntermediateResult(m2::RectD const & viewportRect,
                     FeatureType const & feature,
                     string const & displayName,
                     int matchPenalty,
                     int minVisibleScale);


  bool operator < (IntermediateResult const & o) const;

  Result GenerateFinalResult() const;

  static double ResultDistance(m2::RectD const & viewportRect,
                               m2::RectD const & featureRect);

private:
  string m_str;
  m2::RectD m_rect;
  uint32_t m_type;
  int m_matchPenalty;
  int m_minVisibleScale;
  double m_distance;
};

}  // namespace search::impl
}  // namespace search
