#include "testing/testing.hpp"

#include "routing/routing_quality/waypoints.hpp"

using namespace routing_quality;

// In most cases a passby road should be preferred in case of going pass a city.
// Test on such cases should be grouped in this file.
namespace
{
UNIT_TEST(RoutingQuality_RussiaZelegonrad2Domodedovo)
{
  // From Zelenograd to Domodedovo. MKAD should be preferred.
  TEST(CheckCarRoute({55.98301, 37.21141} /* start */, {55.42081, 37.89361} /* finish */,
                     {{{55.99751, 37.23804}, // Through M-11 and MKAD.
                       {56.00719, 37.28533},
                       {55.88759, 37.48068},
                       {55.83513, 37.39569},
                       {55.57103, 37.69203}},
                      {{55.99775, 37.24941}, // Through M-10 and MKAD.
                       {55.88627, 37.43915},
                       {55.86882, 37.40784},
                       {55.58645, 37.71672},
                       {55.57855, 37.75468}}} /* reference tracks */),
       ());
}

UNIT_TEST(RoutingQuality_BelarusKobrin)
{
  // Test on using a passby road around Kobirn.
  TEST(CheckCarRoute({52.18429, 24.20225} /* start */, {52.24404, 24.45842} /* finish */,
                     {{{52.18694, 24.39903}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_BelarusBobruisk)
{
  TEST(CheckCarRoute({53.24596, 28.93816} /* start */, {53.04386, 29.58098} /* finish */,
                     {{{53.24592, 29.29409}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_RussiaStPetersburg)
{
  TEST(CheckCarRoute({60.08634, 30.10277} /* start */, {59.94584, 30.57703} /* finish */,
                     {{{60.03478, 30.44084}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_BelarusMinsk)
{
  TEST(CheckCarRoute({53.75958, 28.005} /* start */, {54.03957, 26.83097} /* finish */,
                     {{{53.70668, 27.4487}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_BelarusMinskMKAD)
{
  TEST(CheckCarRoute({53.81784, 27.76789} /* start */, {53.94655, 27.36398} /* finish */,
                     {{{53.95037, 27.65361}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_EnglandLondon)
{
  TEST(CheckCarRoute({51.90356, -0.20133} /* start */, {51.23253, -0.33076} /* finish */,
                     {{{51.57098, -0.53503}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_UkraineChernigov)
{
  TEST(CheckCarRoute({51.29419, 31.25718} /* start */, {51.62678, 31.21787} /* finish */,
                     {{{51.48362, 31.18757}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_PolandSiedlce)
{
  TEST(CheckCarRoute({52.17525, 22.19702} /* start */, {52.119802, 22.35855} /* finish */,
                     {{{52.14355, 22.231}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_HungarySzolnok)
{
  TEST(CheckCarRoute({47.18462, 20.04432} /* start */, {47.17919, 20.33486} /* finish */,
                     {{{47.14467, 20.17032}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_USATexasAbilene)
{
  TEST(CheckCarRoute({32.46041, -99.93058} /* start */, {32.43085, -99.59475} /* finish */,
                     {{{32.49038, -99.7269}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_ItalyParma)
{
  TEST(CheckCarRoute({44.81937, 10.2403} /* start */, {44.78228, 10.38824} /* finish */,
                     {{{44.81625, 10.34545}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_SlovenijaLjubljana)
{
  TEST(CheckCarRoute({45.99272, 14.59186} /* start */, {46.10318, 14.46829} /* finish */,
                     {{{46.04449, 14.44669}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_FrancePoitiers)
{
  TEST(CheckCarRoute({46.63612, 0.35762} /* start */, {46.49, 0.36787} /* finish */,
                     {{{46.58706, 0.39232}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_FranceLoudun)
{
  TEST(CheckCarRoute({47.03437, 0.04437} /* start */, {46.97887, 0.09692} /* finish */,
                     {{{47.00307, 0.06713}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_FranceDoueIaFontaine)
{
  TEST(CheckCarRoute({47.22972, -0.30962} /* start */, {47.17023, -0.2185} /* finish */,
                     {{{47.19117, -0.31334}}} /* reference point */),
       ());
}

UNIT_TEST(RoutingQuality_BelgiumBrussel)
{
  TEST(CheckCarRoute({50.88374, 4.2195} /* start */, {50.91494, 4.38122} /* finish */,
                     {{{50.91727, 4.36858}}} /* reference point */),
       ());
}
}  // namespace
