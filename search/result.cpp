#include "result.hpp"

namespace search
{

Result::Result(string const & str, uint32_t featureType, m2::RectD const & featureRect,
               double distanceFromCenter, double directionFromCenter)
  : m_str(str), m_featureRect(featureRect), m_featureType(featureType),
    m_distanceFromCenter(distanceFromCenter), m_directionFromCenter(directionFromCenter)
{
}

Result::Result(string const & str, string const & suggestionStr)
  : m_str(str), m_suggestionStr(suggestionStr)
{
}

Result::ResultType Result::GetResultType() const
{
  if (!m_suggestionStr.empty())
    return RESULT_SUGGESTION;
  return RESULT_FEATURE;
}

m2::RectD Result::GetFeatureRect() const
{
  ASSERT_EQUAL(GetResultType(), RESULT_FEATURE, ());
  return m_featureRect;
}

uint32_t Result::GetFetureType() const
{
  ASSERT_EQUAL(GetResultType(), RESULT_FEATURE, ());
  return m_featureType;
}

double Result::GetDistanceFromCenter() const
{
  ASSERT_EQUAL(GetResultType(), RESULT_FEATURE, ());
  return m_distanceFromCenter;
}

string Result::GetSuggestionString() const
{
  ASSERT_EQUAL(GetResultType(), RESULT_SUGGESTION, ());
  return m_suggestionStr;
}

}  // namespace search
