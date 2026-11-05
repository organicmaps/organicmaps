#include "testing/testing.hpp"

#include "routing/router.hpp"

#include <string>

namespace router_tests
{
UNIT_TEST(RouterType_Parsing)
{
  for (int i = 0; i < static_cast<int>(routing::RouterType::Count); ++i)
  {
    auto const type = static_cast<routing::RouterType>(i);
    auto const name = routing::ToString(type);
    TEST_EQUAL(routing::FromString(name), type, (name));
  }

  for (std::string const name : {"", "count", "future-router", "Pedestrian", "pedestrian "})
    TEST_EQUAL(routing::FromString(name), routing::RouterType::Vehicle, (name));
}
}  // namespace router_tests
