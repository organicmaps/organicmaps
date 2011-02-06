#include "feature_merger.hpp"

//#include "../base/logging.hpp"

#define MAX_MERGED_POINTS_COUNT 10000

FeatureBuilder1Merger::FeatureBuilder1Merger(FeatureBuilder1 const & fb)
  : FeatureBuilder1(fb)
{
}

bool FeatureBuilder1Merger::MergeWith(FeatureBuilder1 const & fb)
{
  // check that both features are of linear type
  if (!fb.m_bLinear || !m_bLinear)
    return false;

  // check that classificator types are the same
  if (fb.m_Types != m_Types)
    return false;

  // do not create too long features
  if (m_Geometry.size() > MAX_MERGED_POINTS_COUNT)
    return false;
  if (fb.m_Geometry.size() > MAX_MERGED_POINTS_COUNT)
    return false;

  // check last-first points equality
  //if (m2::AlmostEqual(m_Geometry.back(), fb.m_Geometry.front()))
  if (m_Geometry.back() == fb.m_Geometry.front())
  {
    // merge fb at the end
    for (size_t i = 1; i < fb.m_Geometry.size(); ++i)
    {
      m_Geometry.push_back(fb.m_Geometry[i]);
      m_LimitRect.Add(m_Geometry.back());
    }
  }
  // check first-last points equality
  //else if (m2::AlmostEqual(m_Geometry.front(), fb.m_Geometry.back()))
  else if (m_Geometry.front() == fb.m_Geometry.back())
  {
    // merge fb in the beginning
    m_Geometry.insert(m_Geometry.begin(), fb.m_Geometry.begin(), fb.m_Geometry.end());
    for (size_t i = 0; i < fb.m_Geometry.size() - 1; ++i)
      m_LimitRect.Add(fb.m_Geometry[i]);
  }
  else
    return false; // no common points were found...


  //static int counter = 0;
  // @TODO check if we got AREA feature after merging, this can be useful for coastlines
  //LOG(LINFO, (++counter, "features were merged!"));

  return true;
}

