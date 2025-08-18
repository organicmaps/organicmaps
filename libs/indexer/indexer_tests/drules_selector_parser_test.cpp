#include "testing/testing.hpp"

#include "indexer/drules_selector.hpp"
#include "indexer/drules_selector_parser.hpp"

#include <string>
#include <vector>

namespace drules_selector_parser_test
{
using namespace drule;
using namespace std;

UNIT_TEST(TestDruleSelectorIsSet)
{
  SelectorExpression e;
  TEST(ParseSelector("name", e), ());

  TEST_EQUAL("name", e.m_tag, ());
  TEST_EQUAL("", e.m_value, ());
  TEST_EQUAL(SelectorOperatorIsSet, e.m_operator, ());
}

UNIT_TEST(TestDruleSelectorIsSet2)
{
  SelectorExpression e;
  TEST(ParseSelector("bbox_area", e), ());

  TEST_EQUAL("bbox_area", e.m_tag, ());
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

UNIT_TEST(TestDruleSelectorIsNotSet2)
{
  SelectorExpression e;
  TEST(ParseSelector("!bbox_area", e), ());

  TEST_EQUAL("bbox_area", e.m_tag, ());
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

UNIT_TEST(TestDruleSelectorLess2)
{
  SelectorExpression e;
  TEST(ParseSelector("bbox_area<1000", e), ());

  TEST_EQUAL("bbox_area", e.m_tag, ());
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

UNIT_TEST(TestDruleSelectorGreater2)
{
  SelectorExpression e;
  TEST(ParseSelector("bbox_area>1000", e), ());

  TEST_EQUAL("bbox_area", e.m_tag, ());
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
  char const * const badFormats[] = {
      "",         "=badformat", "!=badformat",   ">badformat", "<badformat", ">=badformat", "<=badformat",
      "bad$name", "!bad$name",  "bad$name=1000",
  };

  for (auto e : badFormats)
  {
    SelectorExpression expr;
    TEST_EQUAL(false, ParseSelector(e, expr), ("string is", e));
  }
}

UNIT_TEST(PopulationSelector_Smoke)
{
  TEST(ParseSelector("population<1000"), ());
  TEST(ParseSelector(vector<string>({"population>1000"})), ());
  TEST(ParseSelector(vector<string>({"population>=1000", "population<=1000000"})), ());
}

UNIT_TEST(NameSelector_Smoke)
{
  TEST(ParseSelector("name"), ());
  TEST(ParseSelector("!name"), ());
}

UNIT_TEST(InvalidSelector_Smoke)
{
  TEST(!ParseSelector(""), ());
  TEST(!ParseSelector(vector<string>({""})), ());
  TEST(!ParseSelector(vector<string>({"population>=1000", "population<=1000000", ""})), ());
}

}  // namespace drules_selector_parser_test
