#include "testing/testing.hpp"

#include "base/small_set.hpp"

#include <algorithm>
#include <iterator>
#include <vector>

using namespace base;

namespace
{
UNIT_TEST(SmallSet_Empty)
{
  SmallSet<0> set;
  TEST_EQUAL(set.Size(), 0, ());
}

UNIT_TEST(SmallSet_Smoke)
{
  SmallSet<300> set;
  TEST_EQUAL(set.Size(), 0, ());
  for (uint64_t i = 0; i < 300; ++i)
    TEST(!set.Contains(i), ());

  set.Insert(0);
  TEST_EQUAL(set.Size(), 1, ());
  TEST(set.Contains(0), ());

  set.Insert(0);
  TEST_EQUAL(set.Size(), 1, ());
  TEST(set.Contains(0), ());

  set.Insert(5);
  TEST_EQUAL(set.Size(), 2, ());
  TEST(set.Contains(0), ());
  TEST(set.Contains(5), ());

  set.Insert(64);
  TEST_EQUAL(set.Size(), 3, ());
  TEST(set.Contains(0), ());
  TEST(set.Contains(5), ());
  TEST(set.Contains(64), ());

  {
    auto cur = set.begin();
    auto end = set.end();
    for (uint64_t i : {0, 5, 64})
    {
      TEST(cur != end, ());
      TEST_EQUAL(*cur, i, ());
      ++cur;
    }
    TEST(cur == end, ());
  }

  set.Remove(5);
  TEST_EQUAL(set.Size(), 2, ());
  TEST(set.Contains(0), ());
  TEST(!set.Contains(5), ());
  TEST(set.Contains(64), ());

  set.Insert(297);
  set.Insert(298);
  set.Insert(299);
  TEST_EQUAL(set.Size(), 5, ());

  {
    std::vector<uint64_t> const actual(set.begin(), set.end());
    std::vector<uint64_t> const expected = {0, 64, 297, 298, 299};
    TEST_EQUAL(actual, expected, ());
  }

  TEST_EQUAL(set.Size(), std::distance(set.begin(), set.end()), ());
}
}  // namespace
