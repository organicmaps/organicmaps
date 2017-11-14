#include "testing/testing.hpp"

#include "search/approximate_string_match.hpp"

#include "indexer/search_delimiters.hpp"

#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

using namespace search;
using namespace std;
using namespace strings;

namespace
{
struct MatchCostMock
{
  uint32_t Cost10(char) const { return 1; }
  uint32_t Cost01(char) const { return 1; }
  uint32_t Cost11(char, char) const { return 1; }
  uint32_t Cost12(char a, char const * pB) const
  {
    if (a == 'X' && pB[0] == '>' && pB[1] == '<')
      return 0;
    return 2;
  }
  uint32_t Cost21(char const * pA, char b) const { return Cost12(b, pA); }
  uint32_t Cost22(char const *, char const *) const { return 2; }
  uint32_t SwapCost(char, char) const { return 1; }
};

uint32_t FullMatchCost(char const * a, char const * b, uint32_t maxCost = 1000)
{
  return StringMatchCost(a, strlen(a), b, strlen(b), MatchCostMock(), maxCost);
}

uint32_t PrefixMatchCost(char const * a, char const * b)
{
  return StringMatchCost(a, strlen(a), b, strlen(b), MatchCostMock(), 1000, true);
}

void TestEqual(vector<UniString> const v, char const * arr[])
{
  for (size_t i = 0; i < v.size(); ++i)
  {
    TEST_EQUAL(ToUtf8(v[i]), arr[i], ());
    TEST_EQUAL(v[i], MakeUniString(arr[i]), ());
  }
}
}  // namespace

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
