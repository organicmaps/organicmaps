#include "testing/testing.hpp"

#include "integration_tests/osrm_test_tools.hpp"

namespace
{
UNIT_TEST(OnlineRussiaNorthToSouthTest)
{
  shared_ptr<integration::OsrmRouterComponents> routerComponents = integration::GetAllMaps();
  TestOnlineCrosses({34.45, 61.76}, {38.94, 45.07},
                    {"Russia_Central", "Russia_Southern", "Russia_Northwestern"}, routerComponents);
}
}  // namespace
