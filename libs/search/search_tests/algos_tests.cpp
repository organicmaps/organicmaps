#include "testing/testing.hpp"

#include "search/algos.hpp"

#include <algorithm>
#include <iterator>
#include <vector>

using namespace std;

namespace
{
struct CompWithEqual
{
  bool Less(int i1, int i2) const { return i1 <= i2; }
  bool Greater(int i1, int i2) const { return i1 >= i2; }
};

void TestLongestSequence(int in[], size_t inSz, int eta[])
{
  vector<int> res;
  search::LongestSubsequence(vector<int>(in, in + inSz), back_inserter(res), CompWithEqual());
  reverse(res.begin(), res.end());
  TEST(equal(res.begin(), res.end(), eta), (res));
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
