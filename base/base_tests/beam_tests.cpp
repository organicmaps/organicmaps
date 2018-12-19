#include "testing/testing.hpp"

#include "base/assert.hpp"
#include "base/beam.hpp"

#include "base/scope_guard.hpp"
#include "base/timer.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

using namespace base;
using namespace std;

namespace
{
template <template <typename, typename> class Beam>
void Smoke(string const & beamType)
{
  size_t const kCapacity = 100;
  size_t const kTotal = 1000;

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
}  // namespace

UNIT_TEST(Beam_Smoke)
{
  Smoke<base::Beam>("Vector-based");
  Smoke<base::HeapBeam>("Heap-based");
}
