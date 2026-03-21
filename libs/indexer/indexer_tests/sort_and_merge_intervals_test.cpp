#include "indexer/feature_covering.hpp"
#include "testing/testing.hpp"

#include <vector>

UNIT_TEST(SortAndMergeIntervals_1Interval)
{
  std::vector<std::pair<int64_t, int64_t>> v;
  v.push_back(std::make_pair(1ULL, 2ULL));
  TEST_EQUAL(covering::SortAndMergeIntervals(v), v, ());
}
UNIT_TEST(SortAndMergeIntervals_2NotSortedNotOverlappin)
{
  std::vector<std::pair<int64_t, int64_t>> v;
  v.push_back(std::make_pair(3ULL, 4ULL));
  v.push_back(std::make_pair(1ULL, 2ULL));
  std::vector<std::pair<int64_t, int64_t>> e;
  e.push_back(std::make_pair(1ULL, 2ULL));
  e.push_back(std::make_pair(3ULL, 4ULL));
  TEST_EQUAL(covering::SortAndMergeIntervals(v), e, ());
}

UNIT_TEST(SortAndMergeIntervals_BorderMerge)
{
  std::vector<std::pair<int64_t, int64_t>> v;
  v.push_back(std::make_pair(1ULL, 2ULL));
  v.push_back(std::make_pair(2ULL, 3ULL));
  std::vector<std::pair<int64_t, int64_t>> e;
  e.push_back(std::make_pair(1ULL, 3ULL));
  TEST_EQUAL(covering::SortAndMergeIntervals(v), e, ());
}

UNIT_TEST(SortAndMergeIntervals_Overlap)
{
  std::vector<std::pair<int64_t, int64_t>> v;
  v.push_back(std::make_pair(1ULL, 3ULL));
  v.push_back(std::make_pair(2ULL, 4ULL));
  std::vector<std::pair<int64_t, int64_t>> e;
  e.push_back(std::make_pair(1ULL, 4ULL));
  TEST_EQUAL(covering::SortAndMergeIntervals(v), e, ());
}

UNIT_TEST(SortAndMergeIntervals_Contain)
{
  std::vector<std::pair<int64_t, int64_t>> v;
  v.push_back(std::make_pair(2ULL, 3ULL));
  v.push_back(std::make_pair(1ULL, 4ULL));
  std::vector<std::pair<int64_t, int64_t>> e;
  e.push_back(std::make_pair(1ULL, 4ULL));
  TEST_EQUAL(covering::SortAndMergeIntervals(v), e, ());
}

UNIT_TEST(SortAndMergeIntervals_ContainAndTouchBorder)
{
  std::vector<std::pair<int64_t, int64_t>> v;
  v.push_back(std::make_pair(1ULL, 3ULL));
  v.push_back(std::make_pair(1ULL, 4ULL));
  std::vector<std::pair<int64_t, int64_t>> e;
  e.push_back(std::make_pair(1ULL, 4ULL));
  TEST_EQUAL(covering::SortAndMergeIntervals(v), e, ());
}
