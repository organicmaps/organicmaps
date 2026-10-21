#include "testing/testing.hpp"

#include "base/stats.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace stats_tests
{
UNIT_TEST(AverageStats_EmptyAndNegativeSamples)
{
  base::AverageStats<int> stats;
  TEST_EQUAL(stats.GetCount(), 0, ());
  TEST_EQUAL(stats.GetTotal(), 0, ());
  TEST_EQUAL(stats.GetMin(), 0, ());
  TEST_EQUAL(stats.GetMax(), 0, ());
  TEST_EQUAL(stats.GetAverage(), 0.0, ());

  stats(-8);
  stats(-2);
  stats(-5);
  TEST_EQUAL(stats.GetCount(), 3, ());
  TEST_EQUAL(stats.GetTotal(), -15, ());
  TEST_EQUAL(stats.GetMin(), -8, ());
  TEST_EQUAL(stats.GetMax(), -2, ());
  TEST_EQUAL(stats.GetAverage(), -5.0, ());
}

UNIT_TEST(AverageStats_Merge)
{
  base::AverageStats<int> stats;
  base::AverageStats<int> const empty;
  stats.Merge(empty);
  TEST_EQUAL(stats.GetCount(), 0, ());

  base::AverageStats<int> first;
  first(3);
  first(5);
  stats.Merge(first);
  TEST_EQUAL(stats.GetMin(), 3, ());
  TEST_EQUAL(stats.GetMax(), 5, ());
  stats.Merge(empty);
  TEST_EQUAL(stats.GetCount(), 2, ());

  base::AverageStats<int> second;
  second(-7);
  second(11);
  stats.Merge(second);
  TEST_EQUAL(stats.GetCount(), 4, ());
  TEST_EQUAL(stats.GetTotal(), 12, ());
  TEST_EQUAL(stats.GetMin(), -7, ());
  TEST_EQUAL(stats.GetMax(), 11, ());
  TEST_EQUAL(stats.GetAverage(), 3.0, ());
  TEST_EQUAL(first.GetCount(), 2, ());
  TEST_EQUAL(second.GetCount(), 2, ());
}

UNIT_TEST(AverageStats_LargeCountAndSelfMerge)
{
  base::AverageStats<uint64_t> stats;
  stats(1);
  for (int i = 0; i < 33; ++i)
    stats.Merge(stats);
  TEST_EQUAL(stats.GetCount(), uint64_t{1} << 33, ());
  TEST_EQUAL(stats.GetTotal(), uint64_t{1} << 33, ());
  TEST_EQUAL(stats.GetMin(), 1, ());
  TEST_EQUAL(stats.GetMax(), 1, ());
  TEST_EQUAL(stats.GetAverage(), 1.0, ());
}

UNIT_TEST(StatsCollector_NamedSamplesAndSnapshot)
{
  base::StatsCollector<int> collector;
  TEST(collector.GetStats().empty(), ());
  {
    std::string const name = "prefix.mesh.suffix";
    collector.Add(std::string_view(name).substr(7, 4), 2);
  }
  auto const snapshot = collector.GetStats();
  collector.Add("mesh", 6);
  base::AverageStats<int> batch;
  batch(-1);
  batch(9);
  collector.Add("mesh", batch);
  collector.Add("other", 0);

  auto const stats = collector.GetStats();
  TEST_EQUAL(snapshot.size(), 1, ());
  TEST_EQUAL(snapshot.at("mesh").GetCount(), 1, ());
  TEST_EQUAL(snapshot.at("mesh").GetTotal(), 2, ());
  TEST_EQUAL(stats.size(), 2, ());
  auto const & mesh = stats.at("mesh");
  TEST_EQUAL(mesh.GetCount(), 4, ());
  TEST_EQUAL(mesh.GetTotal(), 16, ());
  TEST_EQUAL(mesh.GetMin(), -1, ());
  TEST_EQUAL(mesh.GetMax(), 9, ());
  TEST_EQUAL(mesh.GetAverage(), 4.0, ());
  TEST_EQUAL(stats.at("other").GetCount(), 1, ());
}

UNIT_TEST(StatsCollector_ConcurrentSamplesAndBatches)
{
  base::StatsCollector<uint64_t> collector;
  std::vector<std::thread> threads;
  for (int worker = 0; worker < 4; ++worker)
  {
    threads.emplace_back([&collector]
    {
      base::AverageStats<uint64_t> batch;
      for (uint64_t sample = 0; sample < 1000; ++sample)
      {
        collector.Add("samples", sample);
        batch(sample);
      }
      collector.Add("batches", batch);
    });
  }
  for (auto & thread : threads)
    thread.join();

  auto const stats = collector.GetStats();
  TEST_EQUAL(stats.size(), 2, ());
  for (auto const & [name, entry] : stats)
  {
    TEST_EQUAL(entry.GetCount(), 4000, (name));
    TEST_EQUAL(entry.GetTotal(), 4 * 499500, (name));
    TEST_EQUAL(entry.GetMin(), 0, (name));
    TEST_EQUAL(entry.GetMax(), 999, (name));
    TEST_EQUAL(entry.GetAverage(), 499.5, (name));
  }
}
}  // namespace stats_tests
