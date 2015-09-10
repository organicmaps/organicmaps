
#include "testing/testing.hpp"

#include "indexer/drules_selector.hpp"
#include "indexer/drules_selector_parser.hpp"

using namespace drule;

UNIT_TEST(TestDruleSelectorIsSet)
{
  SelectorExpression e;
  TEST(ParseSelector("name", e), ());

  TEST_EQUAL("name", e.m_tag, ());
  TEST_EQUAL("", e.m_value, ());
  TEST_EQUAL(SelectorOperatorIsSet, e.m_operator, ());
}

UNIT_TEST(TestDruleSelectorIsNotSet)
{
  SelectorExpression e;
  TEST(ParseSelector("!name", e), ());

  TEST_EQUAL("name", e.m_tag, ());
  TEST_EQUAL("", e.m_value, ());
  TEST_EQUAL(SelectorOperatorIsNotSet, e.m_operator, ());
}

UNIT_TEST(TestDruleSelectorEqual)
{
  SelectorExpression e;
  TEST(ParseSelector("population=1000", e), ());

  TEST_EQUAL("population", e.m_tag, ());
  TEST_EQUAL("1000", e.m_value, ());
  TEST_EQUAL(SelectorOperatorEqual, e.m_operator, ());
}

UNIT_TEST(TestDruleSelectorNotEqual)
{
  SelectorExpression e;
  TEST(ParseSelector("population!=1000", e), ());

  TEST_EQUAL("population", e.m_tag, ());
  TEST_EQUAL("1000", e.m_value, ());
  TEST_EQUAL(SelectorOperatorNotEqual, e.m_operator, ());
}

UNIT_TEST(TestDruleSelectorLess)
{
  SelectorExpression e;
  TEST(ParseSelector("population<1000", e), ());

  TEST_EQUAL("population", e.m_tag, ());
  TEST_EQUAL("1000", e.m_value, ());
  TEST_EQUAL(SelectorOperatorLess, e.m_operator, ());
}

UNIT_TEST(TestDruleSelectorGreater)
{
  SelectorExpression e;
  TEST(ParseSelector("population>1000", e), ());

  TEST_EQUAL("population", e.m_tag, ());
  TEST_EQUAL("1000", e.m_value, ());
  TEST_EQUAL(SelectorOperatorGreater, e.m_operator, ());
}

UNIT_TEST(TestDruleSelectorLessOrEqual)
{
  SelectorExpression e;
  TEST(ParseSelector("population<=1000", e), ());

  TEST_EQUAL("population", e.m_tag, ());
  TEST_EQUAL("1000", e.m_value, ());
  TEST_EQUAL(SelectorOperatorLessOrEqual, e.m_operator, ());
}

UNIT_TEST(TestDruleSelectorGreaterOrEqual)
{
  SelectorExpression e;
  TEST(ParseSelector("population>=1000", e), ());

  TEST_EQUAL("population", e.m_tag, ());
  TEST_EQUAL("1000", e.m_value, ());
  TEST_EQUAL(SelectorOperatorGreaterOrEqual, e.m_operator, ());
}

UNIT_TEST(TestDruleSelectorInvalid)
{
  char const * const badFormats[] =
  {
    "",
    "=badformat",
    "!=badformat",
    ">badformat",
    "<badformat",
    ">=badformat",
    "<=badformat",
    "bad$name",
    "!bad$name",
    "bad$name=1000",
  };
  size_t const n = sizeof(badFormats) / sizeof(badFormats[0]);

  for (size_t i = 0; i < n; ++i)
  {
    SelectorExpression e;
    TEST_EQUAL(false, ParseSelector(badFormats[i], e), ("string is", badFormats[i]));
  }
}

UNIT_TEST(TestDruleParseSelectorValid1)
{
  auto selector = ParseSelector("population<1000");
  TEST(selector != nullptr, ());
}

UNIT_TEST(TestDruleParseSelectorValid2)
{
  auto selector = ParseSelector(vector<string>({"population>1000"}));
  TEST(selector != nullptr, ());
}

UNIT_TEST(TestDruleParseSelectorValid3)
{
  auto selector = ParseSelector(vector<string>({"population>=1000","population<=1000000"}));
  TEST(selector != nullptr, ());
}

UNIT_TEST(TestDruleParseSelectorInvalid1)
{
  auto selector = ParseSelector("");
  TEST(selector == nullptr, ());
}

UNIT_TEST(TestDruleParseSelectorInvalid2)
{
  auto selector = ParseSelector(vector<string>({""}));
  TEST(selector == nullptr, ());
}

UNIT_TEST(TestDruleParseSelectorInvalid3)
{
  auto selector = ParseSelector(vector<string>({"population>=1000","population<=1000000", ""}));
  TEST(selector == nullptr, ());
}
