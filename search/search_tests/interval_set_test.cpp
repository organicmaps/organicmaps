#include "testing/testing.hpp"

#include "search/interval_set.hpp"

#include "std/initializer_list.hpp"

using namespace my;

namespace
{
template <typename TElem>
using TInterval = typename IntervalSet<TElem>::TInterval;

template <typename TElem>
void CheckSet(IntervalSet<TElem> const & actual, initializer_list<TInterval<TElem>> intervals)
{
  set<TInterval<TElem>> expected(intervals);
  TEST_EQUAL(actual.Elems(), expected, ());
}
}  // namespace

UNIT_TEST(IntervalSet_Add)
{
  IntervalSet<int> set;
  TEST(set.Elems().empty(), ());

  set.Add(TInterval<int>(0, 2));
  CheckSet(set, {TInterval<int>(0, 2)});

  set.Add(TInterval<int>(1, 3));
  CheckSet(set, {TInterval<int>(0, 3)});

  set.Add(TInterval<int>(-2, 0));
  CheckSet(set, {TInterval<int>(-2, 3)});

  set.Add(TInterval<int>(-4, -3));
  CheckSet(set, {TInterval<int>(-4, -3), TInterval<int>(-2, 3)});

  set.Add(TInterval<int>(7, 10));
  CheckSet(set, {TInterval<int>(-4, -3), TInterval<int>(-2, 3), TInterval<int>(7, 10)});

  set.Add(TInterval<int>(-3, -2));
  CheckSet(set, {TInterval<int>(-4, 3), TInterval<int>(7, 10)});

  set.Add(TInterval<int>(2, 8));
  CheckSet(set, {TInterval<int>(-4, 10)});

  set.Add(TInterval<int>(2, 3));
  CheckSet(set, {TInterval<int>(-4, 10)});
}

UNIT_TEST(IntervalSet_AdjacentIntervalAdd)
{
  IntervalSet<int> set;
  TEST(set.Elems().empty(), ());

  set.Add(TInterval<int>(100, 106));
  CheckSet(set, {TInterval<int>(100, 106)});

  set.Add(TInterval<int>(106, 110));
  CheckSet(set, {TInterval<int>(100, 110)});

  set.Add(TInterval<int>(90, 100));
  CheckSet(set, {TInterval<int>(90, 110)});
}

UNIT_TEST(IntervalSet_SubtractFrom)
{
  IntervalSet<int> set;
  TEST(set.Elems().empty(), ());

  set.Add(TInterval<int>(0, 2));
  set.Add(TInterval<int>(4, 7));
  set.Add(TInterval<int>(10, 11));

  CheckSet(set, {TInterval<int>(0, 2), TInterval<int>(4, 7), TInterval<int>(10, 11)});

  {
    vector<TInterval<int>> difference;
    set.SubtractFrom(TInterval<int>(1, 5), difference);
    vector<TInterval<int>> expected{TInterval<int>(2, 4)};
    TEST_EQUAL(difference, expected, ());
  }

  {
    vector<TInterval<int>> difference;
    set.SubtractFrom(TInterval<int>(-10, -5), difference);
    vector<TInterval<int>> expected{TInterval<int>(-10, -5)};
    TEST_EQUAL(difference, expected, ());
  }

  {
    vector<TInterval<int>> difference;
    set.SubtractFrom(TInterval<int>(0, 11), difference);
    vector<TInterval<int>> expected{TInterval<int>(2, 4), TInterval<int>(7, 10)};
    TEST_EQUAL(difference, expected, ());
  }

  {
    vector<TInterval<int>> difference;
    set.SubtractFrom(TInterval<int>(-1, 11), difference);
    vector<TInterval<int>> expected{TInterval<int>(-1, 0), TInterval<int>(2, 4),
                                    TInterval<int>(7, 10)};
    TEST_EQUAL(difference, expected, ());
  }

  {
    vector<TInterval<int>> difference;
    set.SubtractFrom(TInterval<int>(5, 7), difference);
    TEST(difference.empty(), ());
  }

  {
    vector<TInterval<int>> difference;
    set.SubtractFrom(TInterval<int>(4, 7), difference);
    TEST(difference.empty(), ());
  }

  {
    vector<TInterval<int>> difference;
    set.SubtractFrom(TInterval<int>(3, 7), difference);
    vector<TInterval<int>> expected{TInterval<int>(3, 4)};
    TEST_EQUAL(difference, expected, ());
  }
}
