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
  {
    using TValue = pair<int, int>;

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
  {
    using TValue = pair<int, int>;
    vector<TValue> actual = {{1, 2}, {1, 3}, {2, 100}, {3, 7}, {3, 8}, {2, 500}};
    actual.erase(unique(actual.begin(), actual.end(), my::EqualsBy(&TValue::first)), actual.end());

    vector<int> const expected = {{1, 2, 3, 2}};
    TEST_EQUAL(expected.size(), actual.size(), ());
    for (size_t i = 0; i < actual.size(); ++i)
      TEST_EQUAL(expected[i], actual[i].first, ());
  }

  {
    vector<Int> actual;
    for (auto const v : {0, 0, 1, 2, 2, 0})
      actual.emplace_back(v);
    actual.erase(unique(actual.begin(), actual.end(), my::EqualsBy(&Int::Get)), actual.end());

    vector<int> const expected = {{0, 1, 2, 0}};
    TEST_EQUAL(expected.size(), actual.size(), ());
    for (size_t i = 0; i < actual.size(); ++i)
      TEST_EQUAL(expected[i], actual[i].Get(), ());
  }
}

UNIT_TEST(SortUnique)
{
  vector<int> v = {1, 2, 1, 4, 3, 5, 2, 7, 1};
  my::SortUnique(v);
  vector<int> const expected = {1, 2, 3, 4, 5, 7};
  TEST_EQUAL(v, expected, ());
}

UNIT_TEST(SortUniqueCompPredTest1)
{
  using TValue = int;
  using TPair = pair<TValue, int>;
  vector<TPair> v =
      {{1, 22}, {2, 33}, {1, 23}, {4, 54}, {3, 34}, {5, 23}, {2, 23}, {7, 32}, {1, 12}};

  my::SortUnique<TPair>(v, my::CompareBy(&TPair::first),
                        [](TPair const & p1, TPair const & p2) { return p1.first == p2.first; });

  vector<TValue> const expected = {1, 2, 3, 4, 5, 7};
  TEST_EQUAL(v.size(), expected.size(), ());
  for (int i = 0; i < v.size(); ++i)
    TEST_EQUAL(v[i].first, expected[i], (i));
}

UNIT_TEST(SortUniqueCompPredTest2)
{
  using TValue = double;
  using TPair = pair<TValue, int>;
  vector<TPair> v =
      {{0.5, 11}, {1000.99, 234}, {0.5, 23}, {1234.56789, 54}, {1000.99, 34}};

  my::SortUnique<TPair>(v, my::CompareBy(&TPair::first),
                        [](TPair const & p1, TPair const & p2) { return p1.first == p2.first; });

  vector<TValue> const expected = {0.5, 1000.99, 1234.56789};
  TEST_EQUAL(v.size(), expected.size(), ());
  for (int i = 0; i < v.size(); ++i)
    TEST_EQUAL(v[i].first, expected[i], (i));
}
}  // namespace
