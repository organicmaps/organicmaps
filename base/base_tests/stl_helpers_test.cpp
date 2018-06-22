#include "testing/testing.hpp"

#include "base/stl_helpers.hpp"

#include <algorithm>
#include <deque>
#include <utility>
#include <vector>

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

template <template <typename...> class Cont>
void TestSortUnique()
{
  {
    Cont<int> actual = {1, 2, 1, 4, 3, 5, 2, 7, 1};
    my::SortUnique(actual);
    Cont<int> const expected = {1, 2, 3, 4, 5, 7};
    TEST_EQUAL(actual, expected, ());
  }
  {
    using Value = int;
    using Pair = std::pair<Value, int>;
    Cont<Pair> d =
        {{1, 22}, {2, 33}, {1, 23}, {4, 54}, {3, 34}, {5, 23}, {2, 23}, {7, 32}, {1, 12}};

    my::SortUnique(d, my::LessBy(&Pair::first), my::EqualsBy(&Pair::first));

    Cont<Value> const expected = {1, 2, 3, 4, 5, 7};
    TEST_EQUAL(d.size(), expected.size(), ());
    for (size_t i = 0; i < d.size(); ++i)
      TEST_EQUAL(d[i].first, expected[i], (i));
  }
  {
    using Value = double;
    using Pair = std::pair<Value, int>;
    Cont<Pair> d =
        {{0.5, 11}, {1000.99, 234}, {0.5, 23}, {1234.56789, 54}, {1000.99, 34}};

    my::SortUnique(d, my::LessBy(&Pair::first), my::EqualsBy(&Pair::first));

    Cont<Value> const expected = {0.5, 1000.99, 1234.56789};
    TEST_EQUAL(d.size(), expected.size(), ());
    for (size_t i = 0; i < d.size(); ++i)
      TEST_EQUAL(d[i].first, expected[i], (i));
  }
}

template <template <typename...> class Cont>
void TestEqualsBy()
{
  {
    using Value = std::pair<int, int>;
    Cont<Value> actual = {{1, 2}, {1, 3}, {2, 100}, {3, 7}, {3, 8}, {2, 500}};
    actual.erase(std::unique(actual.begin(), actual.end(), my::EqualsBy(&Value::first)), actual.end());

    Cont<int> const expected = {{1, 2, 3, 2}};
    TEST_EQUAL(expected.size(), actual.size(), ());
    for (size_t i = 0; i < actual.size(); ++i)
      TEST_EQUAL(expected[i], actual[i].first, ());
  }

  {
    Cont<Int> actual;
    for (auto const v : {0, 0, 1, 2, 2, 0})
      actual.emplace_back(v);
    actual.erase(std::unique(actual.begin(), actual.end(), my::EqualsBy(&Int::Get)), actual.end());

    Cont<int> const expected = {{0, 1, 2, 0}};
    TEST_EQUAL(expected.size(), actual.size(), ());
    for (size_t i = 0; i < actual.size(); ++i)
      TEST_EQUAL(expected[i], actual[i].Get(), ());
  }
}

UNIT_TEST(LessBy)
{
  {
    using Value = std::pair<int, int>;

    std::vector<Value> v = {{2, 2}, {0, 4}, {3, 1}, {4, 0}, {1, 3}};
    std::sort(v.begin(), v.end(), my::LessBy(&Value::first));
    for (size_t i = 0; i < v.size(); ++i)
      TEST_EQUAL(i, static_cast<size_t>(v[i].first), ());

    std::vector<Value const *> pv;
    for (auto const & p : v)
      pv.push_back(&p);

    std::sort(pv.begin(), pv.end(), my::LessBy(&Value::second));
    for (size_t i = 0; i < pv.size(); ++i)
      TEST_EQUAL(i, static_cast<size_t>(pv[i]->second), ());
  }

  {
    std::vector<Int> v;
    for (int i = 9; i >= 0; --i)
      v.emplace_back(i);

    std::sort(v.begin(), v.end(), my::LessBy(&Int::Get));
    for (size_t i = 0; i < v.size(); ++i)
      TEST_EQUAL(v[i].Get(), static_cast<int>(i), ());
  }
}

UNIT_TEST(EqualsBy_VectorTest)
{
  TestEqualsBy<std::vector>();
  TestEqualsBy<std::deque>();
}

UNIT_TEST(SortUnique_VectorTest)
{
  TestSortUnique<std::vector>();
  TestSortUnique<std::deque>();
}

UNIT_TEST(IgnoreFirstArgument)
{
  {
    int s = 0;
    auto f1 = [&](int a, int b) { s += a + b; };
    auto f2 = [&](int a, int b) { s -= a + b; };
    auto f3 = my::MakeIgnoreFirstArgument(f2);

    f1(2, 3);
    TEST_EQUAL(s, 5, ());
    f3(1, 2, 3);
    TEST_EQUAL(s, 0, ());
  }

  {
    auto f1 = [](int a, int b) -> int { return a + b; };
    auto f2 = my::MakeIgnoreFirstArgument(f1);

    auto const x = f1(2, 3);
    auto const y = f2("ignored", 2, 3);
    TEST_EQUAL(x, y, ());
  }
}
}  // namespace
