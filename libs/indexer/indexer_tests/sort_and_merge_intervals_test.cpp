#include "indexer/feature_covering.hpp"
#include "testing/testing.hpp"

#include <vector>

using namespace std;

UNIT_TEST(SortAndMergeIntervals_1Interval)
{
  vector<pair<int64_t, int64_t>> v;
  v.push_back(make_pair(1ULL, 2ULL));
  TEST_EQUAL(covering::SortAndMergeIntervals(v), v, ());
}
UNIT_TEST(SortAndMergeIntervals_2NotSortedNotOverlappin)
{
  vector<pair<int64_t, int64_t>> v;
  v.push_back(make_pair(3ULL, 4ULL));
  v.push_back(make_pair(1ULL, 2ULL));
  vector<pair<int64_t, int64_t>> e;
  e.push_back(make_pair(1ULL, 2ULL));
  e.push_back(make_pair(3ULL, 4ULL));
  TEST_EQUAL(covering::SortAndMergeIntervals(v), e, ());
}

UNIT_TEST(SortAndMergeIntervals_BorderMerge)
{
  vector<pair<int64_t, int64_t>> v;
  v.push_back(make_pair(1ULL, 2ULL));
  v.push_back(make_pair(2ULL, 3ULL));
  vector<pair<int64_t, int64_t>> e;
  e.push_back(make_pair(1ULL, 3ULL));
  TEST_EQUAL(covering::SortAndMergeIntervals(v), e, ());
}

UNIT_TEST(SortAndMergeIntervals_Overlap)
{
  vector<pair<int64_t, int64_t>> v;
  v.push_back(make_pair(1ULL, 3ULL));
  v.push_back(make_pair(2ULL, 4ULL));
  vector<pair<int64_t, int64_t>> e;
  e.push_back(make_pair(1ULL, 4ULL));
  TEST_EQUAL(covering::SortAndMergeIntervals(v), e, ());
}

UNIT_TEST(SortAndMergeIntervals_Contain)
{
  vector<pair<int64_t, int64_t>> v;
  v.push_back(make_pair(2ULL, 3ULL));
  v.push_back(make_pair(1ULL, 4ULL));
  vector<pair<int64_t, int64_t>> e;
  e.push_back(make_pair(1ULL, 4ULL));
  TEST_EQUAL(covering::SortAndMergeIntervals(v), e, ());
}

UNIT_TEST(SortAndMergeIntervals_ContainAndTouchBorder)
{
  vector<pair<int64_t, int64_t>> v;
  v.push_back(make_pair(1ULL, 3ULL));
  v.push_back(make_pair(1ULL, 4ULL));
  vector<pair<int64_t, int64_t>> e;
  e.push_back(make_pair(1ULL, 4ULL));
  TEST_EQUAL(covering::SortAndMergeIntervals(v), e, ());
}
