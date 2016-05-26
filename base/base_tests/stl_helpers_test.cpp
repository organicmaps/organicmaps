#include "testing/testing.hpp"

#include "base/stl_helpers.hpp"

#include "std/algorithm.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace
{
class Int
{
public:
  explicit Int(int v) : m_v(v) {}

  inline int Get() const { return m_v; }

private:
  int m_v;
};

UNIT_TEST(LessBy)
{
  using TValue = pair<int, int>;

  {
    vector<TValue> v = {{2, 2}, {0, 4}, {3, 1}, {4, 0}, {1, 3}};
    sort(v.begin(), v.end(), my::LessBy(&TValue::first));
    for (size_t i = 0; i < v.size(); ++i)
      TEST_EQUAL(i, v[i].first, ());

    vector<TValue const *> pv;
    for (auto const & p : v)
      pv.push_back(&p);

    sort(pv.begin(), pv.end(), my::LessBy(&TValue::second));
    for (size_t i = 0; i < pv.size(); ++i)
      TEST_EQUAL(i, pv[i]->second, ());
  }

  {
    vector<Int> v;
    for (int i = 9; i >= 0; --i)
      v.emplace_back(i);

    sort(v.begin(), v.end(), my::LessBy(&Int::Get));
    for (size_t i = 0; i < v.size(); ++i)
      TEST_EQUAL(v[i].Get(), static_cast<int>(i), ());
  }
}

UNIT_TEST(EqualsBy)
{
  using TValue = pair<int, int>;
  vector<TValue> actual = {{1, 2}, {1, 3}, {2, 100}, {3, 7}, {3, 8}, {2, 500}};
  actual.erase(unique(actual.begin(), actual.end(), my::EqualsBy(&TValue::first)), actual.end());

  vector<int> expected = {{1, 2, 3, 2}};
  TEST_EQUAL(expected.size(), actual.size(), ());
  for (size_t i = 0; i < actual.size(); ++i)
    TEST_EQUAL(expected[i], actual[i].first, ());
}
}  // namespace
