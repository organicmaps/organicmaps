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

UNIT_TEST(SortUniquePred)
{
  struct Foo
  {
    int i, j;
  };

  vector<Foo> v = {{1, 22}, {2, 33}, {1, 23}, {4, 54}, {3, 34}, {5, 23}, {2, 23}, {7, 32}, {1, 12}};
  my::SortUnique<Foo>([](Foo const & f1, Foo const & f2) { return f1.i < f2.i; }, v);

  TEST_EQUAL(v.size(), 6, ());
  TEST_EQUAL(v[0].i, 1, ());
  TEST_EQUAL(v[1].i, 2, ());
  TEST_EQUAL(v[2].i, 3, ());
  TEST_EQUAL(v[3].i, 4, ());
  TEST_EQUAL(v[4].i, 5, ());
  TEST_EQUAL(v[5].i, 7, ());
}
}  // namespace
