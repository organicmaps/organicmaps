#include "testing/testing.hpp"

#include "base/enumerate.hpp"

#include "std/map.hpp"
#include "std/vector.hpp"

UNIT_TEST(enumerate)
{
  {
    map<size_t, int> result;
    for (auto const p : my::enumerate(std::vector<int>{1, 2, 3}))
      result.insert(p);
    TEST_EQUAL(result, (map<size_t, int>{{0, 1}, {1, 2}, {2, 3}}), ());
  }

  {
    map<size_t, int> result;
    for (auto const p : my::enumerate(std::vector<int>{1, 2, 3}, 10))
      result.insert(p);
    TEST_EQUAL(result, (map<size_t, int>{{10, 1}, {11, 2}, {12, 3}}), ());
  }

  {
    std::vector<int> const vec{1, 2, 3};
    map<size_t, int> result;
    for (auto const p : my::enumerate(vec))
      result.insert(p);
    TEST_EQUAL(result, (map<size_t, int>{{0, 1}, {1, 2}, {2, 3}}), ());
  }

  {
    std::vector<int> vec{1, 2, 3, 4, 5, 6};
    for (auto const p : my::enumerate(vec, -6))
      p.item *= p.index;

    TEST_EQUAL(vec, (std::vector<int>{-6, -10, -12, -12, -10, -6}), ());
  }
}
