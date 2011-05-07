#include "feature_merger.hpp"

#include "../base/logging.hpp"

#define MAX_MERGED_POINTS_COUNT 10000

FeatureBuilder1Merger::FeatureBuilder1Merger(FeatureBuilder1 const & fb)
  : FeatureBuilder1(fb)
{
}

bool FeatureBuilder1Merger::ReachedMaxPointsCount() const
{
  return (m_Geometry.size() > MAX_MERGED_POINTS_COUNT);
}

void FeatureBuilder1Merger::AppendFeature(FeatureBuilder1Merger const & fb)
{
  // check that both features are of linear type
  CHECK(fb.m_bLinear && m_bLinear, ("Not linear feature"));

  // check that classificator types are the same
  CHECK_EQUAL(fb.m_Types, m_Types, ("Not equal types"));

  // check last-first points equality
  //CHECK_EQUAL(m_Geometry.back(), fb.m_Geometry.front(), ("End and Start point are no equal"));
  CHECK(m_Geometry.back().EqualDxDy(fb.m_Geometry.front(), MercatorBounds::GetCellID2PointAbsEpsilon()),
        (m_Geometry.back(), fb.m_Geometry.front()));

  // merge fb at the end
  size_t const size = fb.m_Geometry.size();
  for (size_t i = 1; i < size; ++i)
    AddPoint(fb.m_Geometry[i]);
}
