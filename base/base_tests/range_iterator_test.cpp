#include "testing/testing.hpp"

#include "base/range_iterator.hpp"

#include "std/vector.hpp"

UNIT_TEST(RangeIterator)
{
  using namespace base;

  {
    vector<int> result;
    for (auto const i : range(5))
      result.push_back(i);
    TEST_EQUAL(result, (vector<int>{0, 1, 2, 3, 4}), ());
  }

  {
    vector<int> result;
    for (auto const i : range(2, 5))
      result.push_back(i);
    TEST_EQUAL(result, (vector<int>{2, 3, 4}), ());
  }

  {
    vector<int> result;
    for (auto const i : reverse_range(5))
      result.push_back(i);
    TEST_EQUAL(result, (vector<int>{4, 3, 2, 1, 0}), ());
  }

  {
    vector<int> result;
    for (auto const i : reverse_range(2, 5))
      result.push_back(i);
    TEST_EQUAL(result, (vector<int>{4, 3, 2}), ());
  }

  {
    TEST_EQUAL(std::vector<int>(MakeRangeIterator(0), MakeRangeIterator(5)),
               (std::vector<int>{0, 1, 2, 3, 4}), ());
  }
}
