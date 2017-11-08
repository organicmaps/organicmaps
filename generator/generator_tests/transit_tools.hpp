#pragma once

#include "testing/testing.hpp"

#include <cstddef>
#include <vector>

namespace routing
{
namespace transit
{
template<typename Obj>
void TestForEquivalence(std::vector<Obj> const & actual, std::vector<Obj> const & expected)
{
  TEST_EQUAL(actual.size(), expected.size(), ());
  for (size_t i = 0; i < actual.size(); ++i)
    TEST(actual[i].IsEqualForTesting(expected[i]), (i, actual[i], expected[i]));
}
}  // namespace transit
}  // namespace routing
