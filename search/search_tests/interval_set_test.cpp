#include "testing/testing.hpp"

#include "search/interval_set.hpp"

#include <initializer_list>
#include <set>
#include <vector>

using namespace base;

namespace
{
template <typename Elem>
using Interval = typename search::IntervalSet<Elem>::Interval;

template <typename Elem>
void CheckSet(search::IntervalSet<Elem> const & actual, std::initializer_list<Interval<Elem>> intervals)
{
  std::set<Interval<Elem>> expected(intervals);
  TEST_EQUAL(actual.Elems(), expected, ());
}
}  // namespace

UNIT_TEST(IntervalSet_Add)
{
  search::IntervalSet<int> set;
  TEST(set.Elems().empty(), ());

  set.Add(Interval<int>(0, 2));
  CheckSet(set, {Interval<int>(0, 2)});

  set.Add(Interval<int>(1, 3));
  CheckSet(set, {Interval<int>(0, 3)});

  set.Add(Interval<int>(-2, 0));
  CheckSet(set, {Interval<int>(-2, 3)});

  set.Add(Interval<int>(-4, -3));
  CheckSet(set, {Interval<int>(-4, -3), Interval<int>(-2, 3)});

  set.Add(Interval<int>(7, 10));
  CheckSet(set, {Interval<int>(-4, -3), Interval<int>(-2, 3), Interval<int>(7, 10)});

  set.Add(Interval<int>(-3, -2));
  CheckSet(set, {Interval<int>(-4, 3), Interval<int>(7, 10)});

  set.Add(Interval<int>(2, 8));
  CheckSet(set, {Interval<int>(-4, 10)});

  set.Add(Interval<int>(2, 3));
  CheckSet(set, {Interval<int>(-4, 10)});
}

UNIT_TEST(IntervalSet_AdjacentIntervalAdd)
{
  search::IntervalSet<int> set;
  TEST(set.Elems().empty(), ());

  set.Add(Interval<int>(100, 106));
  CheckSet(set, {Interval<int>(100, 106)});

  set.Add(Interval<int>(106, 110));
  CheckSet(set, {Interval<int>(100, 110)});

  set.Add(Interval<int>(90, 100));
  CheckSet(set, {Interval<int>(90, 110)});
}

UNIT_TEST(IntervalSet_SubtractFrom)
{
  search::IntervalSet<int> set;
  TEST(set.Elems().empty(), ());

  set.Add(Interval<int>(0, 2));
  set.Add(Interval<int>(4, 7));
  set.Add(Interval<int>(10, 11));

  CheckSet(set, {Interval<int>(0, 2), Interval<int>(4, 7), Interval<int>(10, 11)});

  {
    std::vector<Interval<int>> difference;
    set.SubtractFrom(Interval<int>(1, 5), difference);
    std::vector<Interval<int>> expected{Interval<int>(2, 4)};
    TEST_EQUAL(difference, expected, ());
  }

  {
    std::vector<Interval<int>> difference;
    set.SubtractFrom(Interval<int>(-10, -5), difference);
    std::vector<Interval<int>> expected{Interval<int>(-10, -5)};
    TEST_EQUAL(difference, expected, ());
  }

  {
    std::vector<Interval<int>> difference;
    set.SubtractFrom(Interval<int>(0, 11), difference);
    std::vector<Interval<int>> expected{Interval<int>(2, 4), Interval<int>(7, 10)};
    TEST_EQUAL(difference, expected, ());
  }

  {
    std::vector<Interval<int>> difference;
    set.SubtractFrom(Interval<int>(-1, 11), difference);
    std::vector<Interval<int>> expected{Interval<int>(-1, 0), Interval<int>(2, 4), Interval<int>(7, 10)};
    TEST_EQUAL(difference, expected, ());
  }

  {
    std::vector<Interval<int>> difference;
    set.SubtractFrom(Interval<int>(5, 7), difference);
    TEST(difference.empty(), ());
  }

  {
    std::vector<Interval<int>> difference;
    set.SubtractFrom(Interval<int>(4, 7), difference);
    TEST(difference.empty(), ());
  }

  {
    std::vector<Interval<int>> difference;
    set.SubtractFrom(Interval<int>(3, 7), difference);
    std::vector<Interval<int>> expected{Interval<int>(3, 4)};
    TEST_EQUAL(difference, expected, ());
  }
}
