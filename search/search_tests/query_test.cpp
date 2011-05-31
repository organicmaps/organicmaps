#include "../../testing/testing.hpp"
#include "../query.hpp"
#include "../../base/string_utils.hpp"
#include "../../std/memcpy.hpp"
#include "../../std/string.hpp"

using search::impl::Query;
using strings::MakeUniString;
using strings::UniString;

UNIT_TEST(QueryParseKeywords_Smoke)
{
  vector<UniString> expected;
  expected.push_back(MakeUniString("minsk"));
  expected.push_back(MakeUniString("belarus"));
  TEST_EQUAL(expected,            Query("minsk belarus ", m2::RectD(), NULL).m_keywords, ());
  TEST_EQUAL(MakeUniString(""),   Query("minsk belarus ", m2::RectD(), NULL).m_prefix, ());
  TEST_EQUAL(expected,            Query("minsk belarus ma", m2::RectD(), NULL).m_keywords, ());
  TEST_EQUAL(MakeUniString("ma"), Query("minsk belarus ma", m2::RectD(), NULL).m_prefix, ());
}

UNIT_TEST(QueryParseKeywords_Empty)
{
  TEST_EQUAL(vector<UniString>(), Query("", m2::RectD(), NULL).m_keywords, ());
  TEST_EQUAL(MakeUniString(""),   Query("", m2::RectD(), NULL).m_prefix, ());
  TEST_EQUAL(vector<UniString>(), Query("Z", m2::RectD(), NULL).m_keywords, ());
  TEST_EQUAL(MakeUniString("z"),  Query("Z", m2::RectD(), NULL).m_prefix, ());
}
