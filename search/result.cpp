#include "result.hpp"

#include "../indexer/classificator.hpp"


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

Result Result::GetEndResult()
{
  Result result("", "");
  ASSERT(result.IsEndMarker(), ());
  return result;
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

m2::PointD Result::GetFeatureCenter() const
{
  ASSERT_EQUAL(GetResultType(), RESULT_FEATURE, ());
  return m_featureRect.Center();
}

uint32_t Result::GetFetureType() const
{
  ASSERT_EQUAL(GetResultType(), RESULT_FEATURE, ());
  return m_featureType;
}

string Result::GetFetureTypeAsString() const
{
  ASSERT_EQUAL(GetResultType(), RESULT_FEATURE, ());

  ClassifObject const * p = classif().GetRoot();

  uint8_t i = 0;
  uint8_t v;
  while (ftype::GetValue(m_featureType, i, v) && i < 2)
  {
    p = p->GetObject(v);
    ++i;
  }

  return p->GetName();
}

double Result::GetDistanceFromCenter() const
{
  ASSERT_EQUAL(GetResultType(), RESULT_FEATURE, ());
  return m_distanceFromCenter;
}

double Result::GetDirectionFromCenter() const
{
  ASSERT_EQUAL(GetResultType(), RESULT_FEATURE, ());
  return m_directionFromCenter;
}

char const * Result::GetSuggestionString() const
{
  ASSERT_EQUAL(GetResultType(), RESULT_SUGGESTION, ());
  return m_suggestionStr.c_str();
}

}  // namespace search
