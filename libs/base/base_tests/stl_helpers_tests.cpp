#include "testing/testing.hpp"

#include "base/stl_helpers.hpp"

#include <algorithm>
#include <deque>
#include <map>
#include <vector>

namespace stl_helpers_tests
{
using namespace base;

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
    SortUnique(actual);
    Cont<int> const expected = {1, 2, 3, 4, 5, 7};
    TEST_EQUAL(actual, expected, ());
  }
  {
    using Value = int;
    using Pair = std::pair<Value, int>;
    Cont<Pair> d = {{1, 22}, {2, 33}, {1, 23}, {4, 54}, {3, 34}, {5, 23}, {2, 23}, {7, 32}, {1, 12}};

    SortUnique(d, LessBy(&Pair::first), EqualsBy(&Pair::first));

    Cont<Value> const expected = {1, 2, 3, 4, 5, 7};
    TEST_EQUAL(d.size(), expected.size(), ());
    for (size_t i = 0; i < d.size(); ++i)
      TEST_EQUAL(d[i].first, expected[i], (i));
  }
  {
    using Value = double;
    using Pair = std::pair<Value, int>;
    Cont<Pair> d = {{0.5, 11}, {1000.99, 234}, {0.5, 23}, {1234.56789, 54}, {1000.99, 34}};

    SortUnique(d, LessBy(&Pair::first), EqualsBy(&Pair::first));

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
    actual.erase(std::unique(actual.begin(), actual.end(), EqualsBy(&Value::first)), actual.end());

    Cont<int> const expected = {{1, 2, 3, 2}};
    TEST_EQUAL(expected.size(), actual.size(), ());
    for (size_t i = 0; i < actual.size(); ++i)
      TEST_EQUAL(expected[i], actual[i].first, ());
  }

  {
    Cont<Int> actual;
    for (auto const v : {0, 0, 1, 2, 2, 0})
      actual.emplace_back(v);
    actual.erase(std::unique(actual.begin(), actual.end(), EqualsBy(&Int::Get)), actual.end());

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
    std::sort(v.begin(), v.end(), LessBy(&Value::first));
    for (size_t i = 0; i < v.size(); ++i)
      TEST_EQUAL(i, static_cast<size_t>(v[i].first), ());

    std::vector<Value const *> pv;
    for (auto const & p : v)
      pv.push_back(&p);

    std::sort(pv.begin(), pv.end(), LessBy(&Value::second));
    for (size_t i = 0; i < pv.size(); ++i)
      TEST_EQUAL(i, static_cast<size_t>(pv[i]->second), ());
  }

  {
    std::vector<Int> v;
    for (int i = 9; i >= 0; --i)
      v.emplace_back(i);

    std::sort(v.begin(), v.end(), LessBy(&Int::Get));
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

namespace
{
struct EqualZero
{
  bool operator()(int x) { return (x == 0); }
};

template <class ContT>
void CheckNoZero(ContT & c, typename ContT::iterator i)
{
  c.erase(i, c.end());
  TEST(find_if(c.begin(), c.end(), EqualZero()) == c.end(), ());
}
}  // namespace

UNIT_TEST(RemoveIfKeepValid)
{
  {
    std::vector<int> v;
    TEST(RemoveIfKeepValid(v.begin(), v.end(), EqualZero()) == v.end(), ());

    v.push_back(1);
    TEST(RemoveIfKeepValid(v.begin(), v.end(), EqualZero()) == v.end(), ());

    v.push_back(1);
    TEST(RemoveIfKeepValid(v.begin(), v.end(), EqualZero()) == v.end(), ());
  }

  {
    std::vector<int> v;
    v.push_back(0);
    TEST(RemoveIfKeepValid(v.begin(), v.end(), EqualZero()) == v.begin(), ());

    v.push_back(0);
    TEST(RemoveIfKeepValid(v.begin(), v.end(), EqualZero()) == v.begin(), ());

    v.push_back(1);
    CheckNoZero(v, RemoveIfKeepValid(v.begin(), v.end(), EqualZero()));
    TEST_EQUAL(v.size(), 1, ());

    v.push_back(1);
    v.push_back(0);
    v.push_back(0);
    CheckNoZero(v, RemoveIfKeepValid(v.begin(), v.end(), EqualZero()));
    TEST_EQUAL(v.size(), 2, ());

    v.push_back(0);
    v.push_back(0);
    v.push_back(1);
    v.push_back(1);
    CheckNoZero(v, RemoveIfKeepValid(v.begin(), v.end(), EqualZero()));
    TEST_EQUAL(v.size(), 4, ());
  }

  {
    std::deque<int> v;
    v.push_back(1);
    v.push_back(0);
    v.push_back(1);
    v.push_back(0);
    v.push_back(1);
    v.push_back(0);
    v.push_back(1);
    CheckNoZero(v, RemoveIfKeepValid(v.begin(), v.end(), EqualZero()));
    TEST_EQUAL(v.size(), 4, ());
  }
}

UNIT_TEST(Map_EmplaceOrAssign)
{
  {
    std::map<std::string, std::string, std::less<>> theMap;

    std::string_view key = "key";
    std::string_view val1 = "value";
    TEST(EmplaceOrAssign(theMap, key, val1).second, ());
    TEST_EQUAL(theMap.find(key)->second, val1, ());

    std::string_view val2 = "some_long_value";
    TEST(!EmplaceOrAssign(theMap, key, val2).second, ());
    TEST_EQUAL(theMap.find(key)->second, val2, ());

    std::string_view val3 = "some_other_long_value";
    TEST(!EmplaceOrAssign(theMap, key, std::string(val3)).second, ());
    TEST_EQUAL(theMap.find(key)->second, val3, ());
  }

  {
    class Obj
    {
      int m_v;

      Obj(Obj const &) = delete;
      Obj & operator=(Obj const &) = delete;

    public:
      Obj(int v) : m_v(v) {}
      Obj(Obj &&) = default;
      Obj & operator=(Obj &&) = default;

      bool operator==(Obj const & o) const { return m_v == o.m_v; }
      bool operator<(Obj const & o) const { return m_v < o.m_v; }
    };

    std::map<Obj, Obj> theMap;

    TEST(EmplaceOrAssign(theMap, Obj(1), Obj(2)).second, ());
    TEST(!EmplaceOrAssign(theMap, Obj(1), Obj(3)).second, ());
    TEST(theMap.find(Obj(1))->second == Obj(3), ());
  }
}

}  // namespace stl_helpers_tests
