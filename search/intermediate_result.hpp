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
  enum ResultType
  {
    RESULT_LATLON,
    RESULT_CATEGORY,
    RESULT_FEATURE
  };

  // For RESULT_FEATURE.
  IntermediateResult(m2::RectD const & viewportRect,
                     FeatureType const & f,
                     string const & displayName, string const & regionName);

  // For RESULT_LATLON.
  IntermediateResult(m2::RectD const & viewportRect, string const & regionName,
                     double lat, double lon, double precision);

  // For RESULT_CATEGORY.
  IntermediateResult(string const & name, string const & completionString, int penalty);

  bool operator < (IntermediateResult const & o) const;

  Result GenerateFinalResult() const;

private:
  static double ResultDistance(m2::PointD const & viewportCenter,
                               m2::PointD const & featureCenter);
  static double ResultDirection(m2::PointD const & viewportCenter,
                                m2::PointD const & featureCenter);

  string m_str, m_completionString, m_region;
  m2::RectD m_rect;
  uint32_t m_type;
  double m_distance;
  double m_direction;
  ResultType m_resultType;
  uint8_t m_searchRank;
};

}  // namespace search::impl
}  // namespace search
