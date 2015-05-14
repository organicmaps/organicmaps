#include "testing/testing.hpp"

#include "integration_tests/osrm_test_tools.hpp"

namespace
{
UNIT_TEST(OnlineRussiaNorthToSouthTest)
{
  integration::OsrmRouterComponents & routerComponents  = integration::GetAllMaps();
  TestOnlineCrosses({34.45, 61.76}, {38.94, 45.07},
                    {"Russia_Central", "Russia_Southern", "Russia_Northwestern"}, routerComponents);
}

UNIT_TEST(OnlineEuropeTestNurnbergToMoscow)
{
  integration::OsrmRouterComponents & routerComponents = integration::GetAllMaps();
  TestOnlineCrosses({11.082, 49.45}, {37.56, 55.74},
                    {"Russia_Central", "Belarus", "Poland", "Germany_Bavaria", "Germany_Saxony"}, routerComponents);
}

UNIT_TEST(OnlineAmericanTestOttawaToWashington)
{
  integration::OsrmRouterComponents & routerComponents = integration::GetAllMaps();
  TestOnlineCrosses({-75.69, 45.38}, {-77.031, 38.91},
                    {"Canada_Ontario", "USA_New York", "USA_Pennsylvania", "USA_Maryland", "USA_District of Columbia"}, routerComponents);
}

UNIT_TEST(OnlineAsiaPhuketToPnompen)
{
  integration::OsrmRouterComponents & routerComponents = integration::GetAllMaps();
  TestOnlineCrosses({98.23, 7.90}, {104.86, 11.56},
                    {"Thailand", "Cambodia"}, routerComponents);
}

UNIT_TEST(OnlineAustraliaCanberraToPerth)
{
  integration::OsrmRouterComponents & routerComponents = integration::GetAllMaps();
  TestOnlineCrosses({151.13, -33.88}, {115.88, -31.974},
                    {"Australia"}, routerComponents);
}
}  // namespace
