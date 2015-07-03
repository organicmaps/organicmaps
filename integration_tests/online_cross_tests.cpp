#include "testing/testing.hpp"

#include "integration_tests/osrm_test_tools.hpp"

namespace
{
// Separately tests only fetcher. Other tests will test both fetcher and raw code.
UNIT_TEST(OnlineCrossFetcherSmokeTest)
{
  integration::OsrmRouterComponents & routerComponents = integration::GetAllMaps();
  TestOnlineFetcher({61.76, 34.45}, {45.07, 38.94},
                    {"Russia_Central", "Russia_Southern", "Russia_Northwestern"}, routerComponents);
}

UNIT_TEST(OnlineRussiaNorthToSouthTest)
{
  integration::OsrmRouterComponents & routerComponents  = integration::GetAllMaps();
  TestOnlineCrosses({61.76, 34.45}, {45.07, 38.94},
                    {"Russia_Central", "Russia_Southern", "Russia_Northwestern"}, routerComponents);
}

UNIT_TEST(OnlineEuropeTestNurnbergToMoscow)
{
  integration::OsrmRouterComponents & routerComponents = integration::GetAllMaps();
  TestOnlineCrosses({49.45, 11.082}, {55.74, 37.56},
                    {"Russia_Central", "Belarus", "Poland", "Germany_Bavaria", "Germany_Saxony"}, routerComponents);
}

UNIT_TEST(OnlineAmericanTestOttawaToWashington)
{
  integration::OsrmRouterComponents & routerComponents = integration::GetAllMaps();
  TestOnlineCrosses({45.38, -75.69}, {38.91, -77.031},
                    {"Canada_Ontario", "USA_New York", "USA_Pennsylvania", "USA_Maryland", "USA_District of Columbia"}, routerComponents);
}

UNIT_TEST(OnlineAsiaPhuketToPnompen)
{
  integration::OsrmRouterComponents & routerComponents = integration::GetAllMaps();
  TestOnlineCrosses({7.90, 98.23}, {11.56, 104.86},
                    {"Thailand", "Cambodia"}, routerComponents);
}

UNIT_TEST(OnlineAustraliaCanberraToPerth)
{
  integration::OsrmRouterComponents & routerComponents = integration::GetAllMaps();
  TestOnlineCrosses({-33.88, 151.13}, {-31.974, 115.88},
                    {"Australia"}, routerComponents);
}
}  // namespace
