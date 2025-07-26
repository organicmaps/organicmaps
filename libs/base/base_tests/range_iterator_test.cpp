#include "testing/testing.hpp"

#include "base/range_iterator.hpp"

#include <vector>

UNIT_TEST(RangeIterator)
{
  using namespace base;

  {
    std::vector<int> result;
    for (auto const i : UpTo(5))
      result.push_back(i);
    TEST_EQUAL(result, (std::vector<int>{0, 1, 2, 3, 4}), ());
  }

  {
    std::vector<int> result;
    for (auto const i : UpTo(2, 5))
      result.push_back(i);
    TEST_EQUAL(result, (std::vector<int>{2, 3, 4}), ());
  }

  {
    std::vector<int> result;
    for (auto const i : DownTo(5))
      result.push_back(i);
    TEST_EQUAL(result, (std::vector<int>{4, 3, 2, 1, 0}), ());
  }

  {
    std::vector<int> result;
    for (auto const i : DownTo(2, 5))
      result.push_back(i);
    TEST_EQUAL(result, (std::vector<int>{4, 3, 2}), ());
  }

  {
    TEST_EQUAL(std::vector<int>(MakeRangeIterator(0), MakeRangeIterator(5)), (std::vector<int>{0, 1, 2, 3, 4}), ());
  }
}
