#include "testing/testing.hpp"

#include "base/small_map.hpp"
#include "base/small_set.hpp"
#include "base/timer.hpp"

#include <algorithm>
#include <iterator>
#include <random>
#include <unordered_map>
#include <vector>

namespace small_set_test
{
using namespace base;

UNIT_TEST(SmallSet_Smoke)
{
  SmallSet<300> set;
  TEST_EQUAL(set.Size(), 0, ());
  for (uint64_t i = 0; i < 300; ++i)
    TEST(!set.Contains(i), ());

  set.Insert(0);
  TEST_EQUAL(set.Size(), 1, ());
  TEST(set.Contains(0), ());

  set.Insert(0);
  TEST_EQUAL(set.Size(), 1, ());
  TEST(set.Contains(0), ());

  set.Insert(5);
  TEST_EQUAL(set.Size(), 2, ());
  TEST(set.Contains(0), ());
  TEST(set.Contains(5), ());

  set.Insert(64);
  TEST_EQUAL(set.Size(), 3, ());
  TEST(set.Contains(0), ());
  TEST(set.Contains(5), ());
  TEST(set.Contains(64), ());

  {
    auto cur = set.begin();
    auto end = set.end();
    for (uint64_t i : {0, 5, 64})
    {
      TEST(cur != end, ());
      TEST_EQUAL(*cur, i, ());
      ++cur;
    }
    TEST(cur == end, ());
  }

  set.Remove(5);
  TEST_EQUAL(set.Size(), 2, ());
  TEST(set.Contains(0), ());
  TEST(!set.Contains(5), ());
  TEST(set.Contains(64), ());

  set.Insert(297);
  set.Insert(298);
  set.Insert(299);
  TEST_EQUAL(set.Size(), 5, ());

  {
    std::vector<uint64_t> const actual(set.begin(), set.end());
    std::vector<uint64_t> const expected = {0, 64, 297, 298, 299};
    TEST_EQUAL(actual, expected, ());
  }

  TEST_EQUAL(set.Size(), std::distance(set.begin(), set.end()), ());
}

bool BenchmarkTimeLessOrNear(uint64_t l, uint64_t r, double relativeTolerance)
{
  return (l < r) || ((l - r) / static_cast<double>(l) < relativeTolerance);
}

#ifndef DEBUG
std::vector<uint32_t> GenerateIndices(uint32_t min, uint32_t max)
{
  std::vector<uint32_t> res;

  std::uniform_int_distribution<uint64_t> randDist(min, max);
  std::random_device randDevice;
  std::mt19937 randEngine(randDevice());

  for (size_t i = 0; i < 10000000; ++i)
    res.push_back(randDist(randEngine));

  return res;
}

UNIT_TEST(SmallMap_Benchmark1)
{
  // 1. Init maps.
  // Dataset is similar to routing::VehicleModel.
  std::unordered_map<uint32_t, bool> uMap = {
      {1, true},    {2, false},  {4, false},   {6, true},   {7, true},    {8, true},    {12, false},
      {15, false},  {26, true},  {30, false},  {36, false}, {43, false},  {54, false},  {57, true},
      {58, true},   {65, true},  {69, true},   {90, true},  {95, false},  {119, false}, {167, true},
      {176, false}, {259, true}, {272, false}, {994, true}, {1054, false}};

  base::SmallMap<uint32_t, bool> sMap(uMap.begin(), uMap.end());

  // 2. Generate indices.
  std::vector<uint32_t> indices = GenerateIndices(1, 1054);

  uint64_t t1, t2;
  uint32_t sum1 = 0, sum2 = 0;

  // 3. Run unordered_map.
  {
    base::HighResTimer timer;
    for (auto i : indices)
      sum1 += (uMap.find(i) != uMap.end() ? 1 : 0);
    t1 = timer.ElapsedMilliseconds();
  }

  // 4. Run SmallMap.
  {
    base::HighResTimer timer;
    for (auto i : indices)
      sum2 += (sMap.Find(i) ? 1 : 0);
    t2 = timer.ElapsedMilliseconds();
  }

  TEST_EQUAL(sum1, sum2, ());
  // At this moment, we have rare t2 > t1 on Linux CI.
  TEST(BenchmarkTimeLessOrNear(t2, t1, 0.3), (t2, t1));
  LOG(LINFO, ("unordered_map time =", t1, "SmallMap time =", t2));
}

UNIT_TEST(SmallMap_Benchmark2)
{
  using namespace std;

  uint32_t i = 0;
  // Dataset is similar to routing::VehicleModelFactory.
  unordered_map<string, shared_ptr<int>> uMap = {
      {"", make_shared<int>(i++)},
      {"Australia", make_shared<int>(i++)},
      {"Austria", make_shared<int>(i++)},
      {"Belarus", make_shared<int>(i++)},
      {"Belgium", make_shared<int>(i++)},
      {"Brazil", make_shared<int>(i++)},
      {"Denmark", make_shared<int>(i++)},
      {"France", make_shared<int>(i++)},
      {"Finland", make_shared<int>(i++)},
      {"Germany", make_shared<int>(i++)},
      {"Hungary", make_shared<int>(i++)},
      {"Iceland", make_shared<int>(i++)},
      {"Netherlands", make_shared<int>(i++)},
      {"Norway", make_shared<int>(i++)},
      {"Oman", make_shared<int>(i++)},
      {"Poland", make_shared<int>(i++)},
      {"Romania", make_shared<int>(i++)},
      {"Russian Federation", make_shared<int>(i++)},
      {"Slovakia", make_shared<int>(i++)},
      {"Spain", make_shared<int>(i++)},
      {"Switzerland", make_shared<int>(i++)},
      {"Turkey", make_shared<int>(i++)},
      {"Ukraine", make_shared<int>(i++)},
      {"United Kingdom", make_shared<int>(i++)},
      {"United States of America", make_shared<int>(i++)},
  };

  base::SmallMap<std::string, std::shared_ptr<int>> sMap(uMap.begin(), uMap.end());

  // 2. Generate indices.
  std::vector<std::string> keys;
  for (auto const & e : uMap)
  {
    keys.push_back(e.first);
    keys.push_back(e.first + "_Foo");
    keys.push_back(e.first + "_Bar");
    keys.push_back(e.first + "_Bazz");
  }
  std::vector<uint32_t> indices = GenerateIndices(0, keys.size() - 1);

  uint64_t t1, t2;
  uint32_t sum1 = 0, sum2 = 0;

  // 3. Run unordered_map.
  {
    base::HighResTimer timer;
    for (auto i : indices)
    {
      auto const it = uMap.find(keys[i]);
      if (it != uMap.end())
        sum1 += *it->second;
    }
    t1 = timer.ElapsedMilliseconds();
  }

  // 4. Run SmallMap.
  {
    base::HighResTimer timer;
    for (auto i : indices)
    {
      auto const * p = sMap.Find(keys[i]);
      if (p)
        sum2 += **p;
    }
    t2 = timer.ElapsedMilliseconds();
  }

  TEST_EQUAL(sum1, sum2, ());
  // std::hash(std::string) is better than std::less(std::string)
  TEST_LESS(t1, t2, ());
  LOG(LINFO, ("unordered_map time =", t1, "SmallMap time =", t2));
}

// Small 4 elements sample doesn't work for new (gcc11+, clang14+) toolchain.
/*
UNIT_TEST(SmallMap_Benchmark3)
{
  // Dataset is similar to routing::VehicleModel.m_surfaceFactors.
  std::unordered_map<int, int> uMap = {
    {1, 0}, {10, 1}, {100, 2}, {1000, 3},
  };

  base::SmallMap<int, int> sMap(uMap.begin(), uMap.end());
  base::SmallMapBase<int, int> sbMap(uMap.begin(), uMap.end());

  std::vector<uint32_t> indices = GenerateIndices(0, 3);
  // Missing key queries are even worse for the std map.
  std::vector<int> keys;
  for (auto const & e : uMap)
    keys.push_back(e.first);

  uint64_t t1, t2, t3;
  uint32_t sum1 = 0, sum2 = 0, sum3 = 0;

  // 3. Run unordered_map.
  {
    base::HighResTimer timer;
    for (auto i : indices)
      sum1 += uMap.find(keys[i])->second;
    t1 = timer.ElapsedMilliseconds();
  }

  // 4. Run SmallMap.
  {
    base::HighResTimer timer;
    for (auto i : indices)
      sum2 += *sMap.Find(keys[i]);
    t2 = timer.ElapsedMilliseconds();
  }

  // 5. Run SmallMapBase.
  {
    base::HighResTimer timer;
    for (auto i : indices)
      sum3 += *sbMap.Find(keys[i]);
    t3 = timer.ElapsedMilliseconds();
  }

  TEST_EQUAL(sum1, sum2, ());
  TEST_EQUAL(sum1, sum3, ());
  TEST_LESS(t2, t1, ());
  TEST(BenchmarkTimeLessOrNear(t3, t2, 0.05), (t3, t2));
  LOG(LINFO, ("unordered_map time =", t1, "SmallMap time =", t2, "SmallMapBase time =", t3));
}
*/
#endif

}  // namespace small_set_test
