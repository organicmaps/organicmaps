#include "testing/testing.hpp"

#include "base/stl_helpers.hpp"

#include "std/algorithm.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

UNIT_TEST(CompareBy_Smoke)
{
  vector<pair<int, int>> v = {{2, 2}, {0, 4}, {3, 1}, {4, 0}, {1, 3}};
  sort(v.begin(), v.end(), my::CompareBy(&pair<int, int>::first));
  for (size_t i = 0; i < v.size(); ++i)
    TEST_EQUAL(i, v[i].first, ());

  vector<pair<int, int> const *> pv;
  for (auto const & p : v)
    pv.push_back(&p);

  sort(pv.begin(), pv.end(), my::CompareBy(&pair<int, int>::second));
  for (size_t i = 0; i < pv.size(); ++i)
    TEST_EQUAL(i, pv[i]->second, ());
}
