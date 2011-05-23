#include "../../testing/testing.hpp"
#include "../string_match.hpp"

#include "../../std/memcpy.hpp"

namespace
{

class TestMatchCost
{
public:
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

uint32_t MatchCost(char const * a, char const * b, uint32_t maxCost = 1000)
{
  return ::search::StringMatchCost(a, strlen(a), b, strlen(b), TestMatchCost(), maxCost);
}

}

UNIT_TEST(StringMatchCost)
{
  TEST_EQUAL(MatchCost("", ""), 0, ());
  TEST_EQUAL(MatchCost("a", "b"), 1, ());
  TEST_EQUAL(MatchCost("a", ""), 1, ());
  TEST_EQUAL(MatchCost("", "b"), 1, ());
  TEST_EQUAL(MatchCost("ab", "cd"), 2, ());
  TEST_EQUAL(MatchCost("ab", "ba"), 1, ());
  TEST_EQUAL(MatchCost("abcd", "efgh"), 4, ());
  TEST_EQUAL(MatchCost("Hello!", "Hello!"), 0, ());
  TEST_EQUAL(MatchCost("Hello!", "Helo!"), 1, ());
  TEST_EQUAL(MatchCost("X", "X"), 0, ());
  TEST_EQUAL(MatchCost("X", "><"), 0, ());
  TEST_EQUAL(MatchCost("XXX", "><><><"), 0, ());
  TEST_EQUAL(MatchCost("XXX", "><X><"), 0, ());
  TEST_EQUAL(MatchCost("TeXt", "Te><t"), 0, ());
  TEST_EQUAL(MatchCost("TeXt", "Te><"), 1, ());
  TEST_EQUAL(MatchCost("TeXt", "TetX"), 1, ());
  TEST_EQUAL(MatchCost("TeXt", "Tet><"), 2, ());
  TEST_EQUAL(MatchCost("", "ALongString"), 11, ());
  TEST_EQUAL(MatchCost("x", "ALongString"), 11, ());
  TEST_EQUAL(MatchCost("g", "ALongString"), 10, ());
}
