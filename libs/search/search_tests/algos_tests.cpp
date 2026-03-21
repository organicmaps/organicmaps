#include "testing/testing.hpp"

#include "search/algos.hpp"

#include <algorithm>
#include <iterator>
#include <vector>

namespace
{
struct CompWithEqual
{
  bool Less(int i1, int i2) const { return i1 <= i2; }
  bool Greater(int i1, int i2) const { return i1 >= i2; }
};

void TestLongestSequence(int in[], size_t inSz, int eta[])
{
  std::vector<int> res;
  search::LongestSubsequence(std::vector<int>(in, in + inSz), std::back_inserter(res), CompWithEqual());
  std::reverse(res.begin(), res.end());
  TEST(std::equal(res.begin(), res.end(), eta), (res));
}
}  // namespace

UNIT_TEST(LS_Smoke)
{
  {
    int arr[] = {1};
    TestLongestSequence(arr, ARRAY_SIZE(arr), arr);
  }

  {
    int arr[] = {1, 2};
    TestLongestSequence(arr, ARRAY_SIZE(arr), arr);
  }

  {
    int arr[] = {2, 1};
    TestLongestSequence(arr, ARRAY_SIZE(arr), arr);
  }

  {
    int arr[] = {1, 9, 2, 3, 8, 0, 7, -1, 7, -2, 7};
    int res[] = {1, 2, 3, 7, 7, 7};
    TestLongestSequence(arr, ARRAY_SIZE(arr), res);
  }
}
