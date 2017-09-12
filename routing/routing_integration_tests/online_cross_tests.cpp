#include "testing/testing.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

namespace
{
// Separately tests only fetcher. Other tests will test both fetcher and raw code.
UNIT_TEST(OnlineCrossFetcherSmokeTest)
{
  integration::IRouterComponents & routerComponents =
      integration::GetVehicleComponents<VehicleType::Car>();
  TestOnlineFetcher({61.76, 34.45}, {45.07, 38.94},
                    {"Russia_Republic of Karelia_South", "Russia_Leningradskaya Oblast_Southeast",
                     "Russia_Novgorod Oblast", "Russia_Tver Oblast", "Russia_Moscow Oblast_West",
                     "Russia_Moscow", "Russia_Moscow Oblast_East", "Russia_Tula Oblast",
                     "Russia_Lipetsk Oblast", "Russia_Voronezh Oblast", "Russia_Rostov Oblast",
                     "Russia_Krasnodar Krai"},
                    routerComponents);
}

UNIT_TEST(OnlineRussiaNorthToSouthTest)
{
  integration::IRouterComponents & routerComponents =
      integration::GetVehicleComponents<VehicleType::Car>();
  TestOnlineCrosses({61.76, 34.45}, {45.07, 38.94},
                    {"Russia_Republic of Karelia_South", "Russia_Leningradskaya Oblast_Southeast",
                     "Russia_Novgorod Oblast", "Russia_Tver Oblast", "Russia_Moscow Oblast_East",
                     "Russia_Moscow Oblast_West", "Russia_Moscow", "Russia_Tula Oblast",
                     "Russia_Lipetsk Oblast", "Russia_Voronezh Oblast", "Russia_Rostov Oblast",
                     "Russia_Krasnodar Krai"},
                    routerComponents);
}

UNIT_TEST(OnlineRoadToSeaCenterTest)
{
  integration::IRouterComponents & routerComponents =
      integration::GetVehicleComponents<VehicleType::Car>();
  TestOnlineCrosses({61.76, 34.45}, {42.25,30.10}, {}, routerComponents);
}

UNIT_TEST(OnlineEuropeTestNurnbergToMoscow)
{
  integration::IRouterComponents & routerComponents =
      integration::GetVehicleComponents<VehicleType::Car>();
  TestOnlineCrosses(
      {49.45, 11.082}, {55.74, 37.56},
      {"Germany_Free State of Bavaria_Middle Franconia",
       "Germany_Free State of Bavaria_Upper Franconia", "Germany_Saxony_Leipzig",
       "Germany_Saxony_Dresden", "Poland_Lower Silesian Voivodeship",
       "Poland_Greater Poland Voivodeship", "Poland_Lodz Voivodeship", "Poland_Lublin Voivodeship",
       "Poland_Masovian Voivodeship", "Belarus_Brest Region", "Belarus_Hrodna Region",
       "Belarus_Minsk Region", "Belarus_Vitebsk Region", "Russia_Smolensk Oblast",
       "Russia_Moscow Oblast_West", "Russia_Moscow"},
      routerComponents);
}

UNIT_TEST(OnlineAmericanTestOttawaToWashington)
{
  integration::IRouterComponents & routerComponents =
      integration::GetVehicleComponents<VehicleType::Car>();
  TestOnlineCrosses({45.38, -75.69}, {38.91, -77.031},
                    {"Canada_Ontario_Kingston", "US_New York_North", "US_New York_West",
                     "US_Pennsylvania_Scranton", "US_Pennsylvania_Central", "US_Maryland_Baltimore",
                     "US_Maryland_and_DC"},
                    routerComponents);
}

UNIT_TEST(OnlineAsiaPhuketToPnompen)
{
  integration::IRouterComponents & routerComponents =
      integration::GetVehicleComponents<VehicleType::Car>();
  TestOnlineCrosses({7.89, 98.30}, {11.56, 104.86}, {"Thailand_South", "Cambodia"},
                    routerComponents);
}

UNIT_TEST(OnlineAustraliaCanberraToPerth)
{
  integration::IRouterComponents & routerComponents =
      integration::GetVehicleComponents<VehicleType::Car>();
  TestOnlineCrosses({-33.88, 151.13}, {-31.974, 115.88},
                    {"Australia_New South Wales", "Australia_Victoria", "Australia_South Australia",
                     "Australia_Western Australia", "Australia_Sydney"},
                    routerComponents);
}
}  // namespace
