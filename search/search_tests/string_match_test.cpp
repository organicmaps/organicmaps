#include "testing/testing.hpp"
#include "search/approximate_string_match.hpp"

#include "search/search_tests/match_cost_mock.hpp"

#include "indexer/search_delimiters.hpp"

#include "base/stl_add.hpp"

#include "std/cstring.hpp"


using namespace search;
using namespace strings;

namespace
{

uint32_t FullMatchCost(char const * a, char const * b, uint32_t maxCost = 1000)
{
  return StringMatchCost(a, strlen(a), b, strlen(b), MatchCostMock<char>(), maxCost);
}

uint32_t PrefixMatchCost(char const * a, char const * b)
{
  return StringMatchCost(a, strlen(a), b, strlen(b), MatchCostMock<char>(), 1000, true);
}

}

UNIT_TEST(StringMatchCost_FullMatch)
{
  TEST_EQUAL(FullMatchCost("", ""), 0, ());
  TEST_EQUAL(FullMatchCost("a", "b"), 1, ());
  TEST_EQUAL(FullMatchCost("a", ""), 1, ());
  TEST_EQUAL(FullMatchCost("", "b"), 1, ());
  TEST_EQUAL(FullMatchCost("ab", "cd"), 2, ());
  TEST_EQUAL(FullMatchCost("ab", "ba"), 1, ());
  TEST_EQUAL(FullMatchCost("abcd", "efgh"), 4, ());
  TEST_EQUAL(FullMatchCost("Hello!", "Hello!"), 0, ());
  TEST_EQUAL(FullMatchCost("Hello!", "Helo!"), 1, ());
  TEST_EQUAL(FullMatchCost("X", "X"), 0, ());
  TEST_EQUAL(FullMatchCost("X", "><"), 0, ());
  TEST_EQUAL(FullMatchCost("XXX", "><><><"), 0, ());
  TEST_EQUAL(FullMatchCost("XXX", "><X><"), 0, ());
  TEST_EQUAL(FullMatchCost("TeXt", "Te><t"), 0, ());
  TEST_EQUAL(FullMatchCost("TeXt", "Te><"), 1, ());
  TEST_EQUAL(FullMatchCost("TeXt", "TetX"), 1, ());
  TEST_EQUAL(FullMatchCost("TeXt", "Tet><"), 2, ());
  TEST_EQUAL(FullMatchCost("", "ALongString"), 11, ());
  TEST_EQUAL(FullMatchCost("x", "ALongString"), 11, ());
  TEST_EQUAL(FullMatchCost("g", "ALongString"), 10, ());
}

UNIT_TEST(StringMatchCost_MaxCost)
{
  TEST_EQUAL(FullMatchCost("g", "ALongString", 1), 2, ());
  TEST_EQUAL(FullMatchCost("g", "ALongString", 5), 6, ());
  TEST_EQUAL(FullMatchCost("g", "ALongString", 9), 10, ());
  TEST_EQUAL(FullMatchCost("g", "ALongString", 9), 10, ());
  TEST_EQUAL(FullMatchCost("g", "ALongString", 10), 10, ());
  TEST_EQUAL(FullMatchCost("g", "ALongString", 11), 10, ());
}

UNIT_TEST(StringMatchCost_PrefixMatch)
{
  TEST_EQUAL(PrefixMatchCost("", "Hello!"), 0, ());
  TEST_EQUAL(PrefixMatchCost("H", "Hello!"), 0, ());
  TEST_EQUAL(PrefixMatchCost("He", "Hello!"), 0, ());
  TEST_EQUAL(PrefixMatchCost("Hel", "Hello!"), 0, ());
  TEST_EQUAL(PrefixMatchCost("Hell", "Hello!"), 0, ());
  TEST_EQUAL(PrefixMatchCost("Hello", "Hello!"), 0, ());
  TEST_EQUAL(PrefixMatchCost("Hello!", "Hello!"), 0, ());
  TEST_EQUAL(PrefixMatchCost("Hx", "Hello!"), 1, ());
  TEST_EQUAL(PrefixMatchCost("Helpo", "Hello!"), 1, ());
  TEST_EQUAL(PrefixMatchCost("Happo", "Hello!"), 3, ());
}

namespace
{

void TestEqual(vector<UniString> const v, char const * arr[])
{
  for (size_t i = 0; i < v.size(); ++i)
  {
    TEST_EQUAL(ToUtf8(v[i]), arr[i], ());
    TEST_EQUAL(v[i], MakeUniString(arr[i]), ());
  }
}

}

UNIT_TEST(StringSplit_Smoke)
{
  vector<UniString> tokens;

  {
    string const s = "1/2";
    UniString const s1 = NormalizeAndSimplifyString(s);
    TEST_EQUAL(ToUtf8(s1), s, ());

    char const * arr[] = { "1", "2" };
    SplitUniString(s1, MakeBackInsertFunctor(tokens), Delimiters());
    TestEqual(tokens, arr);
  }
}
