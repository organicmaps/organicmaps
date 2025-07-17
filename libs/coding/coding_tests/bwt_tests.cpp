#include "testing/testing.hpp"

#include "coding/bwt.hpp"

#include <algorithm>
#include <random>
#include <string>

using namespace coding;
using namespace std;

namespace
{
string RevRevBWT(string const & s)
{
  string r;
  auto const start = BWT(s, r);

  string rr;
  RevBWT(start, r, rr);
  return rr;
}

UNIT_TEST(BWT_Smoke)
{
  {
    TEST_EQUAL(BWT(0 /* n */, nullptr /* s */, nullptr /* r */), 0, ());
  }

  {
    string r;
    TEST_EQUAL(BWT(string() /* s */, r /* r */), 0, ());
  }

  {
    string const s = "aaaaaa";
    string r;
    TEST_EQUAL(BWT(s, r), 5, ());
    TEST_EQUAL(r, s, ());
  }

  {
    string const s = "mississippi";
    string r;
    TEST_EQUAL(BWT(s, r), 4, ());
    TEST_EQUAL(r, "pssmipissii", ());
  }
}

UNIT_TEST(RevBWT_Smoke)
{
  string const strings[] = {"abaaba", "mississippi", "a b b", "Again and again and again"};
  for (auto const & s : strings)
    TEST_EQUAL(s, RevRevBWT(s), ());

  for (size_t i = 0; i < 100; ++i)
  {
    string const s(i, '\0');
    TEST_EQUAL(s, RevRevBWT(s), ());
  }

  for (size_t i = 0; i < 100; ++i)
  {
    string const s(i, 'a' + (i % 3));
    TEST_EQUAL(s, RevRevBWT(s), ());
  }
}

UNIT_TEST(RevBWT_AllBytes)
{
  int kSeed = 42;
  int kMin = 1;
  int kMax = 10;

  mt19937 engine(kSeed);
  uniform_int_distribution<int> uid(kMin, kMax);

  string s;
  for (size_t i = 0; i < 256; ++i)
  {
    auto const count = uid(engine);
    ASSERT_GREATER_OR_EQUAL(count, kMin, ());
    ASSERT_LESS_OR_EQUAL(count, kMax, ());
    for (int j = 0; j < count; ++j)
      s.push_back(static_cast<uint8_t>(i));
  }
  shuffle(s.begin(), s.end(), engine);
  TEST_EQUAL(s, RevRevBWT(s), ());
}
}  // namespace
