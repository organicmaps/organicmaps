#include "../../testing/testing.hpp"
#include "../approximate_string_match.hpp"
#include "match_cost_mock.hpp"
#include "../../std/memcpy.hpp"

namespace
{

uint32_t FullMatchCost(char const * a, char const * b, uint32_t maxCost = 1000)
{
  return ::search::StringMatchCost(a, strlen(a), b, strlen(b),
                                   search::MatchCostMock<char>(), maxCost);
}

uint32_t PrefixMatchCost(char const * a, char const * b)
{
  return ::search::StringMatchCost(a, strlen(a), b, strlen(b),
                                   search::MatchCostMock<char>(), 1000, true);
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
