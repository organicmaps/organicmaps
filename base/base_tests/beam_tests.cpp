#include "testing/testing.hpp"

#include "base/assert.hpp"
#include "base/beam.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace base;
using namespace std;

UNIT_TEST(Beam_Smoke)
{
  size_t const kCapacity = 10;
  size_t const kTotal = 100;

  CHECK_LESS_OR_EQUAL(kCapacity, kTotal, ());
  
  base::Beam<uint32_t, double> beam(kCapacity);

  for (uint32_t i = 0; i < kTotal; ++i)
    beam.Add(i, static_cast<double>(i));

  vector<double> expected;
  for (size_t i = 0; i < kCapacity; ++i)
    expected.emplace_back(kTotal - 1 - i);

  vector<double> actual;
  actual.reserve(kCapacity);
  for (auto const & e : beam.GetEntries())
    actual.emplace_back(e.m_value);

  CHECK_EQUAL(expected, actual, ());
}
