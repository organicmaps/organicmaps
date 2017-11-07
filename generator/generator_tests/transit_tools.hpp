#pragma once

#include "testing/testing.hpp"

#include <vector>

namespace routing
{
namespace transit
{
template<typename Obj>
void TestForEquivalence(std::vector<Obj> const & objects, std::vector<Obj> const & expected)
{
  for (size_t i = 0; i < objects.size(); ++i)
    TEST(objects[i].IsEqualForTesting(expected[i]), (i, objects[i], expected[i]));
}
}  // namespace transit
}  // namespace routing
