#include "testing/testing.hpp"

#include "coding/bwt.hpp"

#include <algorithm>
#include <random>
#include <string>

namespace
{
std::string RevRevBWT(std::string const & s)
{
  std::string r;
  auto const start = coding::BWT(s, r);

  std::string rr;
  coding::RevBWT(start, r, rr);
  return rr;
}

UNIT_TEST(BWT_Smoke)
{
  {
    TEST_EQUAL(coding::BWT(0 /* n */, nullptr /* s */, nullptr /* r */), 0, ());
  }

  {
    std::string r;
    TEST_EQUAL(coding::BWT(std::string() /* s */, r /* r */), 0, ());
  }

  {
    std::string const s = "aaaaaa";
    std::string r;
    TEST_EQUAL(coding::BWT(s, r), 5, ());
    TEST_EQUAL(r, s, ());
  }

  {
    std::string const s = "mississippi";
    std::string r;
    TEST_EQUAL(coding::BWT(s, r), 4, ());
    TEST_EQUAL(r, "pssmipissii", ());
  }
}

UNIT_TEST(RevBWT_Smoke)
{
  std::string const strings[] = {"abaaba", "mississippi", "a b b", "Again and again and again"};
  for (auto const & s : strings)
    TEST_EQUAL(s, RevRevBWT(s), ());

  for (size_t i = 0; i < 100; ++i)
  {
    std::string const s(i, '\0');
    TEST_EQUAL(s, RevRevBWT(s), ());
  }

  for (size_t i = 0; i < 100; ++i)
  {
    std::string const s(i, 'a' + (i % 3));
    TEST_EQUAL(s, RevRevBWT(s), ());
  }
}

UNIT_TEST(RevBWT_AllBytes)
{
  int kSeed = 42;
  int kMin = 1;
  int kMax = 10;

  std::mt19937 engine(kSeed);
  std::uniform_int_distribution<int> uid(kMin, kMax);

  std::string s;
  for (size_t i = 0; i < 256; ++i)
  {
    auto const count = uid(engine);
    ASSERT_GREATER_OR_EQUAL(count, kMin, ());
    ASSERT_LESS_OR_EQUAL(count, kMax, ());
    for (int j = 0; j < count; ++j)
      s.push_back(static_cast<uint8_t>(i));
  }
  std::shuffle(s.begin(), s.end(), engine);
  TEST_EQUAL(s, RevRevBWT(s), ());
}
}  // namespace
