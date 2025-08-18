#include "testing/testing.hpp"

#include "base/assert.hpp"
#include "base/beam.hpp"

#include "base/scope_guard.hpp"
#include "base/timer.hpp"

#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace beam_tests
{
using namespace base;
using namespace std;

template <template <typename, typename> class Beam>
void Smoke()
{
  size_t const kCapacity = 10;
  size_t const kTotal = 100;

  CHECK_LESS_OR_EQUAL(kCapacity, kTotal, ());

  Beam<uint32_t, double> beam(kCapacity);

  for (uint32_t i = 0; i < kTotal; ++i)
    beam.Add(i, static_cast<double>(i));

  vector<double> expected;
  for (size_t i = 0; i < kCapacity; ++i)
    expected.emplace_back(kTotal - 1 - i);

  vector<double> actual;
  actual.reserve(kCapacity);
  for (auto const & e : beam.GetEntries())
    actual.emplace_back(e.m_value);

  sort(actual.rbegin(), actual.rend());
  CHECK_EQUAL(expected, actual, ());
}

template <template <typename, typename> class Beam>
void Benchmark(string const & beamType, uint64_t const numResets, size_t const capacity, uint64_t const numEvents)
{
  base::Timer timer;
  SCOPE_GUARD(timerGuard, [&] { LOG(LINFO, ("type:", beamType, "\ttime passed:", timer.ElapsedSeconds())); });

  CHECK_LESS_OR_EQUAL(capacity, numEvents, ());

  mt19937 rng(0);
  uniform_real_distribution<double> dis(0.0, 1.0);
  for (uint64_t wave = 0; wave <= numResets; ++wave)
  {
    Beam<uint64_t, double> beam(capacity);

    uint64_t const begin = wave * numEvents / (numResets + 1);
    uint64_t const end = (wave + 1) * numEvents / (numResets + 1);
    for (uint64_t i = begin; i < end; ++i)
      beam.Add(i, dis(rng));
  }
}

UNIT_TEST(Beam_Smoke)
{
  Smoke<Beam>();
  Smoke<HeapBeam>();
}

UNIT_TEST(Beam_Benchmark)
{
  size_t const kCapacity = 100;
  uint64_t const kNumEvents = 1000000;

  for (uint64_t numResets = 0; numResets < 1000; numResets += 200)
  {
    LOG(LINFO, ("Resets =", numResets, "Capacity =", kCapacity, "Total events =", kNumEvents));
    Benchmark<Beam>("Vector-based", numResets, kCapacity, kNumEvents);
    Benchmark<HeapBeam>("Heap-based", numResets, kCapacity, kNumEvents);
  }
}
}  // namespace beam_tests
