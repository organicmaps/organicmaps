#include "testing/testing.hpp"

#include "search/approximate_string_match.hpp"

#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

namespace string_match_test
{
using namespace search;
using namespace std;
using namespace strings;

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

void TestEqual(vector<UniString> const & v, base::StringIL const & expected)
{
  TEST_EQUAL(v.size(), expected.size(), (expected));

  size_t i = 0;
  for (auto const & e : expected)
  {
    TEST_EQUAL(ToUtf8(v[i]), e, ());
    TEST_EQUAL(v[i], MakeUniString(e), ());
    ++i;
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

UNIT_TEST(StringSplit_Smoke)
{
  TestEqual(NormalizeAndTokenizeString("1/2"), {"1", "2"});
  TestEqual(NormalizeAndTokenizeString("xxx-yyy"), {"xxx", "yyy"});
}

UNIT_TEST(StringSplit_Apostrophe)
{
  TestEqual(NormalizeAndTokenizeString("Barne's & Noble"), {"barnes", "noble"});
  TestEqual(NormalizeAndTokenizeString("Michael's"), {"michaels"});
  TestEqual(NormalizeAndTokenizeString("'s"), {"s"});
  TestEqual(NormalizeAndTokenizeString("xyz'"), {"xyz"});
  TestEqual(NormalizeAndTokenizeString("'''"), {});
}

UNIT_TEST(StringSplit_NumeroHashtag)
{
  TestEqual(NormalizeAndTokenizeString("Зона №51"), {"зона", "51"});
  TestEqual(NormalizeAndTokenizeString("Зона № 51"), {"зона", "51"});
  TestEqual(NormalizeAndTokenizeString("Area #51"), {"area", "51"});
  TestEqual(NormalizeAndTokenizeString("Area # "), {"area"});
  TestEqual(NormalizeAndTokenizeString("Area #One"), {"area", "one"});
  TestEqual(NormalizeAndTokenizeString("xxx#yyy"), {"xxx", "yyy"});
  TestEqual(NormalizeAndTokenizeString("#'s"), {"s"});
  TestEqual(NormalizeAndTokenizeString("##osm's"), {"osms"});
}

}  // namespace string_match_test
