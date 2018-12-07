#include "testing/testing.hpp"

#include "routing/routing_quality/waypoints.hpp"

using namespace routing_quality;

// Test on preferring better but longer roads should be grouped in this file.
namespace
{
// Secondary should be preferred against residential.
UNIT_TEST(RoutingQuality_RussiaMoscowTushino)
{
  TEST(CheckCarRoute({55.84398, 37.45018} /* start */, {55.85489, 37.43784} /* finish */,
                     {{{55.84343, 37.43949}}} /* reference track */),
       ());
}

UNIT_TEST(RoutingQuality_TurkeyIzmirArea)
{
  TEST(CheckCarRoute({38.80146, 26.97696} /* start */, {39.06835, 26.88686} /* finish */,
                     {{{39.08146, 27.11798}}} /* reference track */),
       ());
}

UNIT_TEST(RoutingQuality_BosniaAndHerzegovina)
{
  TEST(CheckCarRoute({42.71401, 18.30412} /* start */, {42.95101, 18.08966} /* finish */,
                     {{{42.88222,17.9919}}} /* reference track */),
       ());
}

UNIT_TEST(RoutingQuality_CzechiaPrague)
{
  TEST(CheckCarRoute({50.10159, 14.43324} /* start */, {50.20976, 14.43361} /* finish */,
                     {{{50.15078, 14.49205}}} /* reference track */),
       ());
}

UNIT_TEST(RoutingQuality_FinlandHelsinki)
{
  TEST(CheckCarRoute({60.16741, 24.94255} /* start */, {64.13182, 28.38784} /* finish */,
                     {{{60.95453, 25.6951}}} /* reference track */),
       ());
}

UNIT_TEST(RoutingQuality_USAOklahoma)
{
  TEST(CheckCarRoute({35.39166, -97.55402} /* start */, {35.38452, -97.5742} /* finish */,
                     {{{35.39912, -97.57622}}} /* reference track */),
       ());
}

// @TODO This test was broken after using maxspeed tag value always if it's available.
// It should be fixed by using maxspeed taking into account highway class.
// Trunk should be preferred against primary.
//UNIT_TEST(RoutingQuality_IranSouth)
//{
//  TEST(CheckCarRoute({32.45088, 51.76419} /* start */, {32.97067, 51.50399} /* finish */,
//                     {{{32.67021, 51.64323}, {32.68752, 51.63387}}} /* reference track */),
//       ());
//}

UNIT_TEST(RoutingQuality_EindhovenNetherlands)
{
  TEST(CheckCarRoute({50.91974, 5.33535} /* start */, {51.92532, 5.49066} /* finish */,
                     {{{51.42016, 5.42881}, {51.44316, 5.42723}, {51.50230, 5.47485}}} /* reference track */),
       ());
}

UNIT_TEST(RoutingQuality_GeteborgasSweden)
{
  TEST(CheckCarRoute({57.77064, 11.88079} /* start */, {57.71231, 11.93157} /* finish */,
                     {{{57.74912, 11.87343}}} /* reference track */),
       ());
}

UNIT_TEST(RoutingQuality_CigilTurkey)
{
  TEST(CheckCarRoute({38.48175, 27.12952} /* start */, {38.47558, 27.06765} /* finish */,
                     {{{38.4898049, 27.1016266}}} /* reference track */),
       ());
}

UNIT_TEST(RoutingQuality_KatowicePoland)
{
  TEST(CheckCarRoute({50.37282, 18.75667} /* start */, {50.83499, 19.14612} /* finish */,
                     {{{50.422229, 19.04746}, {50.48831, 19.21423}}} /* reference track */),
       ());
}

UNIT_TEST(RoutingQuality_KrasnoyarskBratsk)
{
  TEST(CheckCarRoute({56.009, 92.873} /* start */, {56.163, 101.611} /* finish */,
                     {{{55.89285, 97.99953}, {54.59928, 100.60402}}} /* reference track */),
       ());
}

UNIT_TEST(RoutingQuality_VoronezhSochi)
{
  TEST(CheckCarRoute({51.65487, 39.21293} /* start */, {43.58547, 39.72311} /* finish */,
                     {{{46.14169, 39.85306}, {45.17069, 39.10869},
                       {45.02157, 39.12510}, {44.54344, 38.95853}}} /* reference track */),
       ());
}

UNIT_TEST(RoutingQuality_BerlinkaWarsawPoland)
{
  TEST(CheckCarRoute({54.41616, 20.05675} /* start */, {52.18937, 20.94026} /* finish */,
                     {{{54.24278, 19.66106}, {54.13679, 19.45166},
                       {54.06452, 19.62416}, {53.69769, 19.98204},
                       {53.11194, 20.40002}, {52.62966, 20.38488}}} /* reference track */),
       ());
}

UNIT_TEST(RoutingQuality_LenOblBadPaving)
{
  TEST(CheckCarRoute({60.23884, 29.71603} /* start */, {60.29083, 29.80333} /* finish */,
                     {{{60.2510134, 29.790209}}} /* reference track */),
       ());
}

UNIT_TEST(RoutingQuality_MosOblBadPaving)
{
  TEST(CheckCarRoute({55.93849, 36.02792} /* start */, {55.93566, 36.05074} /* finish */,
                     {{{55.92321, 36.04630}}} /* reference track */),
       ());
}
}  // namespace
