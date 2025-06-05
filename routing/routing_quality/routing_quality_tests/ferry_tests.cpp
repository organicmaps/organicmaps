#include "testing/testing.hpp"

#include "routing/routing_quality/waypoints.hpp"

using namespace routing_quality;

namespace
{
UNIT_TEST(Ferry_RoutingQuality_FinlandBridgeInsteadOfFerry)
{
  TEST(CheckCarRoute({56.11155, 12.81101} /* start */, {55.59857, 12.3069} /* finish */,
                     {{{55.56602, 12.88537}}} /* reference track */),
       ());
}
// TODO: This test doesn't pass because routing::RouteWeight::operator<
// prefer roads with less number of barriers. It will be more useful to consider
// barriers only with access=no/private/etc tag.
// UNIT_TEST(Ferry_RoutingQuality_RussiaToCrimeaFerry)
//{
//  // From Russia to Crimea
//  TEST(CheckCarRoute({45.34123, 36.67679} /* start */, {45.36479, 36.62194} /* finish */,
//                     {{{45.3532, 36.64912}}} /* reference track */),
//       ());
//}

UNIT_TEST(Ferry_RoutingQuality_RussiaFromCrimeaFerry)
{
  TEST(CheckCarRoute({45.36479, 36.62194} /* start */, {45.34123, 36.67679} /* finish */,
                     {{{45.3532, 36.64912}}} /* reference track */),
       ());
}

// For tests Ferry_RoutingQuality_1 - Ferry_RoutingQuality_15
// Look at: https://confluence.mail.ru/display/MAPSME/Ferries for more details.

UNIT_TEST(Ferry_RoutingQuality_1)
{
  TEST(CheckCarRoute({67.89425, 13.00747} /* start */, {67.28024, 14.37397} /* finish */,
                     {{{67.60291, 13.65267}}} /* reference track */),
       ());
}

UNIT_TEST(Ferry_RoutingQuality_2)
{
  TEST(CheckCarRoute({51.91347, 5.24265} /* start */, {51.98885, 5.20058} /* finish */,
                     {{{51.97494, 5.10631}}} /* reference track */),
       ());
}

UNIT_TEST(Ferry_RoutingQuality_3)
{
  TEST(CheckCarRoute({50.68323, 7.15683} /* start */, {50.58042, 7.32259} /* finish */,
                     {{{50.72206, 7.14872}, {50.70747, 7.17801}}} /* reference track */),
       ());
}

UNIT_TEST(Ferry_RoutingQuality_4)
{
  TEST(CheckCarRoute({49.37098, 0.84813} /* start */, {49.47950, 0.80918} /* finish */,
                     {{{49.36829, 0.81359}, {49.41177, 0.79639}}} /* reference track */),
       ());
}

UNIT_TEST(Ferry_RoutingQuality_5)
{
  TEST(CheckCarRoute({53.59885, 9.32285} /* start */, {53.93504, 9.48396} /* finish */,
                     {{{53.55170, 9.89848}}} /* reference track */),
       ());
}

UNIT_TEST(Ferry_RoutingQuality_6)
{
  TEST(CheckCarRoute({48.268548, 21.483862} /* start */, {48.24756, 21.54246} /* finish */,
                     {{{48.32574, 21.56094}}} /* reference track */),
       ());
}

UNIT_TEST(Ferry_RoutingQuality_7)
{
  TEST(CheckCarRoute({52.09319, 25.96368} /* start */, {52.02190, 25.98767} /* finish */,
                     {{{52.08914, 26.13001}}} /* reference track */),
       ());
}

UNIT_TEST(Ferry_RoutingQuality_8)
{
  TEST(CheckCarRoute({53.36914, 17.95644} /* start */, {53.46632, 18.01876} /* finish */,
                     {{{53.31395, 17.98559}}} /* reference track */),
       ());
}

UNIT_TEST(Ferry_RoutingQuality_9)
{
  TEST(CheckCarRoute({52.57264, 14.83948} /* start */, {52.74278, 14.95435} /* finish */,
                     {{{52.65097, 14.99906}}} /* reference track */),
       ());
}

UNIT_TEST(Ferry_RoutingQuality_10)
{
  TEST(CheckCarRoute({63.33900, 10.27831} /* start */, {63.33338, 10.22694} /* finish */,
                     {{{63.32261, 10.26896}}} /* reference track */),
       ());
}

UNIT_TEST(Ferry_RoutingQuality_11)
{
  TEST(CheckCarRoute({56.51167, 10.28726} /* start */, {56.57695, 10.11188} /* finish */,
                     {{{56.44610, 10.26030}}} /* reference track */),
       ());
}

UNIT_TEST(Ferry_RoutingQuality_13)
{
  TEST(CheckCarRoute({51.53923, 11.78523} /* start */, {51.62372, 11.86635} /* finish */,
                     {{{51.50772, 11.94096}}} /* reference track */),
       ());
}

UNIT_TEST(Ferry_RoutingQuality_14)
{
  TEST(CheckCarRoute({52.68467, 16.24031} /* start */, {52.74559, 16.27202} /* finish */,
                     {{{52.71631, 16.37917}}} /* reference track */),
       ());
}

UNIT_TEST(Ferry_RoutingQuality_15)
{
  TEST(CheckCarRoute({48.29162, 22.21412} /* start */, {48.27409, 22.46155} /* finish */,
                     {{{48.43306, 22.23317}}} /* reference track */),
       ());
}
}  // namespace
