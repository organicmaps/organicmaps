#include "testing/testing.hpp"

#include "base/stl_helpers.hpp"

#include "std/algorithm.hpp"
#include "std/deque.hpp"
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

template<template<class, class> class TContainer>
void TestSortUnique()
{
  {
    TContainer<int, allocator<int>> actual = {1, 2, 1, 4, 3, 5, 2, 7, 1};
    my::SortUnique(actual);
    TContainer<int, allocator<int>> const expected = {1, 2, 3, 4, 5, 7};
    TEST_EQUAL(actual, expected, ());
  }
  {
    using TValue = int;
    using TPair = pair<TValue, int>;
    TContainer<TPair, allocator<TPair>> d =
        {{1, 22}, {2, 33}, {1, 23}, {4, 54}, {3, 34}, {5, 23}, {2, 23}, {7, 32}, {1, 12}};

    my::SortUnique<TPair>(d, my::LessBy(&TPair::first), my::EqualsBy(&TPair::first));

    TContainer<TValue, allocator<TValue>> const expected = {1, 2, 3, 4, 5, 7};
    TEST_EQUAL(d.size(), expected.size(), ());
    for (int i = 0; i < d.size(); ++i)
      TEST_EQUAL(d[i].first, expected[i], (i));
  }
  {
    using TValue = double;
    using TPair = pair<TValue, int>;
    TContainer<TPair, allocator<TPair>> d =
        {{0.5, 11}, {1000.99, 234}, {0.5, 23}, {1234.56789, 54}, {1000.99, 34}};

    my::SortUnique<TPair>(d, my::LessBy(&TPair::first), my::EqualsBy(&TPair::first));

    TContainer<TValue, allocator<TValue>> const expected = {0.5, 1000.99, 1234.56789};
    TEST_EQUAL(d.size(), expected.size(), ());
    for (int i = 0; i < d.size(); ++i)
      TEST_EQUAL(d[i].first, expected[i], (i));
  }
}

template<template<class, class> class TContainer>
void TestEqualsBy()
{
  {
    using TValue = pair<int, int>;
    TContainer<TValue, allocator<TValue>> actual = {{1, 2}, {1, 3}, {2, 100}, {3, 7}, {3, 8}, {2, 500}};
    actual.erase(unique(actual.begin(), actual.end(), my::EqualsBy(&TValue::first)), actual.end());

    TContainer<int, allocator<int>> const expected = {{1, 2, 3, 2}};
    TEST_EQUAL(expected.size(), actual.size(), ());
    for (size_t i = 0; i < actual.size(); ++i)
      TEST_EQUAL(expected[i], actual[i].first, ());
  }

  {
    TContainer<Int, allocator<Int>> actual;
    for (auto const v : {0, 0, 1, 2, 2, 0})
      actual.emplace_back(v);
    actual.erase(unique(actual.begin(), actual.end(), my::EqualsBy(&Int::Get)), actual.end());

    TContainer<int, allocator<int>> const expected = {{0, 1, 2, 0}};
    TEST_EQUAL(expected.size(), actual.size(), ());
    for (size_t i = 0; i < actual.size(); ++i)
      TEST_EQUAL(expected[i], actual[i].Get(), ());
  }
}

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

UNIT_TEST(EqualsBy_VectorTest)
{
  TestEqualsBy<vector>();
}

UNIT_TEST(EqualsBy_DequeTest)
{
  TestEqualsBy<deque>();
}

UNIT_TEST(SortUnique_VectorTest)
{
  TestSortUnique<vector>();
}

UNIT_TEST(SortUnique_DequeTest)
{
  TestSortUnique<deque>();
}
}  // namespace
