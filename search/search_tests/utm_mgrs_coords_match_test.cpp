#include "testing/testing.hpp"

#include "search/utm_mgrs_coords_match.hpp"

#include "base/math.hpp"

#include <iostream>

namespace utm_mgrs_coords_match_test
{
using namespace search;

// We expect the results to be quite precise.
double const kEps = 1e-5;

void TestAlmostEqual(std::optional<ms::LatLon> maybeLatLon, double expectedLat, double expectedLon)
{
  TEST(maybeLatLon.has_value(), ());

  auto const actualLat = maybeLatLon.value().m_lat;
  TEST(base::AlmostEqualAbsOrRel(actualLat, expectedLat, kEps), ("Lat is not close", actualLat, expectedLat));

  auto const actualLon = maybeLatLon.value().m_lon;
  TEST(base::AlmostEqualAbsOrRel(actualLon, expectedLon, kEps), ("Lon is not close", actualLon, expectedLon));
}

UNIT_TEST(MatchUTMCoords)
{
  // Extra spaces shouldn't break format
  TEST(MatchUTMCoords("15 N  500000 4649776").has_value(), ());
  TEST(MatchUTMCoords("15 N  500000  4649776").has_value(), ());

  TestAlmostEqual(MatchUTMCoords("15N 500000 4649776"), 42.0, -93.0);
  TestAlmostEqual(MatchUTMCoords("15 N 500000 4649776"), 42.0, -93.0);
  TestAlmostEqual(MatchUTMCoords("32U 294409 5628898"), 50.77535, 6.08389);
  TestAlmostEqual(MatchUTMCoords("32 U 294409 5628898"), 50.77535, 6.08389);
  TestAlmostEqual(MatchUTMCoords("30X 476594 9328501"), 84.0, -5.00601);
  TestAlmostEqual(MatchUTMCoords("30 N 476594 9328501"), 84.0, -5.00601);
}

UNIT_TEST(MatchUTMCoords_False)
{
  double lat, lon;

  TEST(!MatchUTMCoords("2 1st").has_value(), ());
  TEST(!MatchUTMCoords("15N5000004649776").has_value(), ());

  // Wrong zone number (first two digits)
  TEST(!MatchUTMCoords("0X 476594 9328501").has_value(), ());
  TEST(!MatchUTMCoords("0 X 476594 9328501").has_value(), ());
  TEST(!MatchUTMCoords("61N 294409 5628898").has_value(), ());
  TEST(!MatchUTMCoords("61 N 294409 5628898").has_value(), ());

  // Wrong zone letter
  TEST(!MatchUTMCoords("25I 500000 4649776").has_value(), ());
  TEST(!MatchUTMCoords("25 I 500000 4649776").has_value(), ());
  TEST(!MatchUTMCoords("25O 500000 4649776").has_value(), ());
  TEST(!MatchUTMCoords("25 O 500000 4649776").has_value(), ());
  TEST(!MatchUTMCoords("5A 500000 4649776").has_value(), ());
  TEST(!MatchUTMCoords("5 A 500000 4649776").has_value(), ());
  TEST(!MatchUTMCoords("7B 500000 4649776").has_value(), ());
  TEST(!MatchUTMCoords("7 B 500000 4649776").has_value(), ());

  // easting out of range (must be between 100,000 m and 999,999 m)
  TEST(!MatchUTMCoords("19S 999 6360877").has_value(), ());
  TEST(!MatchUTMCoords("19S 99999 6360877").has_value(), ());
  TEST(!MatchUTMCoords("19S 1000000 6360877").has_value(), ());
  TEST(!MatchUTMCoords("19S 2000000 6360877").has_value(), ());

  // northing out of range (must be between 0 m and 10,000,000 m)
  TEST(!MatchUTMCoords("30N 476594 10000001").has_value(), ());
  TEST(!MatchUTMCoords("30N 476594 20000000").has_value(), ());
}

UNIT_TEST(MatchMGRSCoords_parsing)
{
  double lat, lon;

  TEST(MatchMGRSCoords("30N YF 67993 00000").has_value(), ());
  TEST(MatchMGRSCoords("30N YF 67993 00000 ").has_value(), ());
  TEST(MatchMGRSCoords("30N YF 67993  00000 ").has_value(), ());
  TEST(MatchMGRSCoords("30N YF  67993  00000 ").has_value(), ());
  TEST(MatchMGRSCoords("30NYF 67993 00000").has_value(), ());
  TEST(MatchMGRSCoords("30NYF 67993 00000 ").has_value(), ());
  TEST(MatchMGRSCoords("30NYF 67993  00000 ").has_value(), ());
  TEST(MatchMGRSCoords("30NYF67993 00000").has_value(), ());
  TEST(MatchMGRSCoords("30NYF67993 00000 ").has_value(), ());
  TEST(MatchMGRSCoords("30NYF67993  00000 ").has_value(), ());
  TEST(MatchMGRSCoords("30NYF6799300000").has_value(), ());
  TEST(MatchMGRSCoords("30NYF6799300000 ").has_value(), ());

  // Wrong number of digits
  TEST(!MatchMGRSCoords("30NYF 679930000 ").has_value(), ());
  TEST(!MatchMGRSCoords("30NYF 679930000").has_value(), ());

  TEST(!MatchMGRSCoords("30N YF 693 23020").has_value(), ());
  TEST(!MatchMGRSCoords("30N YF 693 23 ").has_value(), ());

  // Invalid zone
  TEST(!MatchMGRSCoords("30 FF 693 230").has_value(), ());
  TEST(!MatchMGRSCoords("30A YF 693 230").has_value(), ());
  TEST(!MatchMGRSCoords("30Z YF 693 230").has_value(), ());
  TEST(!MatchMGRSCoords("30Z F 693 230").has_value(), ());
  TEST(!MatchMGRSCoords("30Z 3F 693 230").has_value(), ());
  TEST(!MatchMGRSCoords("30Z K? 693 230").has_value(), ());
  TEST(!MatchMGRSCoords("30Z IB 693 230").has_value(), ());
  TEST(!MatchMGRSCoords("30Z DO 693 230").has_value(), ());

  // Wrong easting or northing
  TEST(!MatchMGRSCoords("30NYF 679_3 00000").has_value(), ());
  TEST(!MatchMGRSCoords("30NYF 6930&000").has_value(), ());
}

UNIT_TEST(MatchMGRSCoords_convert)
{
  TestAlmostEqual(MatchMGRSCoords("30N YF 67993 00000"), 0.000000, -0.592330);
  TestAlmostEqual(MatchMGRSCoords("31N BA 00000 00000"), 0.000000, 0.304980);
  TestAlmostEqual(MatchMGRSCoords("32R LR 00000 00000"), 27.107980, 6.982490);
  TestAlmostEqual(MatchMGRSCoords("32R MR 00000 00000"), 27.118850, 7.991060);
  TestAlmostEqual(MatchMGRSCoords("33R TK 05023 99880"), 27.089890, 12.025310);
  TestAlmostEqual(MatchMGRSCoords("31R GQ 69322 99158"), 31.596040, 5.838410);
  TestAlmostEqual(MatchMGRSCoords("35L KF 50481 01847"), -13.541120, 24.694510);
  TestAlmostEqual(MatchMGRSCoords("33L YL 59760 01153"), -9.028520, 17.362850);
  TestAlmostEqual(MatchMGRSCoords("36G VR 27441 12148"), -45.040410, 32.078730);
  TestAlmostEqual(MatchMGRSCoords("41R KQ 30678 99158"), 31.596040, 60.161590);
  TestAlmostEqual(MatchMGRSCoords("43F CF 66291 06635"), -49.578080, 73.150400);
  TestAlmostEqual(MatchMGRSCoords("43F DF 65832 14591"), -49.520340, 74.527920);
  TestAlmostEqual(MatchMGRSCoords("41G NL 72559 12148"), -45.040410, 63.921270);
  TestAlmostEqual(MatchMGRSCoords("41G PL 72152 04746"), -45.089800, 65.187670);
  TestAlmostEqual(MatchMGRSCoords("42G UR 00000 00000"), -45.125150, 66.456880);
  TestAlmostEqual(MatchMGRSCoords("42G VR 00000 00000"), -45.146390, 67.727970);
  TestAlmostEqual(MatchMGRSCoords("42G WR 00000 00000"), -45.153480, 69.000000);
  TestAlmostEqual(MatchMGRSCoords("42G XR 00000 00000"), -45.146390, 70.272030);
  TestAlmostEqual(MatchMGRSCoords("42G YR 00000 00000"), -45.125150, 71.543120);
  TestAlmostEqual(MatchMGRSCoords("43G CL 27848 04746"), -45.089800, 72.812330);
  TestAlmostEqual(MatchMGRSCoords("43G DL 27441 12148"), -45.040410, 74.078730);
  TestAlmostEqual(MatchMGRSCoords("41G PR 08066 09951"), -40.554160, 64.276370);
  TestAlmostEqual(MatchMGRSCoords("41G QR 07714 03147"), -40.596410, 65.454770);
  TestAlmostEqual(MatchMGRSCoords("42G UA 00000 00000"), -40.626640, 66.635320);
  TestAlmostEqual(MatchMGRSCoords("43L GF 49519 01847"), -13.541120, 77.305490);
  TestAlmostEqual(MatchMGRSCoords("45L VL 00000 00000"), -9.045430, 86.090120);
  TestAlmostEqual(MatchMGRSCoords("50Q KE 64726 97325"), 18.051740, 114.777360);
  TestAlmostEqual(MatchMGRSCoords("53F LF 66291 06635"), -49.578080, 133.150400);
  TestAlmostEqual(MatchMGRSCoords("60F VL 65832 14591"), -49.520340, 176.527920);

  TestAlmostEqual(MatchMGRSCoords("04X ER 00000 00000"), 81.060880, -159.000000);
  TestAlmostEqual(MatchMGRSCoords("05X MK 95564 95053"), 81.016470, -153.254480);
  TestAlmostEqual(MatchMGRSCoords("08X ME 93476 90354"), 72.012660, -135.189280);
  TestAlmostEqual(MatchMGRSCoords("12H TF 59828 01847"), -36.098350, -113.667820);
  TestAlmostEqual(MatchMGRSCoords("15N YA 67993 00000"), 0.000000, -90.592330);
  TestAlmostEqual(MatchMGRSCoords("16N BF 00000 00000"), 0.000000, -89.695020);
  TestAlmostEqual(MatchMGRSCoords("16N CF 00000 00000"), 0.000000, -88.797050);
  TestAlmostEqual(MatchMGRSCoords("21L TL 40240 01153"), -9.028520, -59.362850);
  TestAlmostEqual(MatchMGRSCoords("24P YV 49519 98153"), 13.541120, -36.694510);
  TestAlmostEqual(MatchMGRSCoords("31K BA 64726 02675"), -18.051740, 0.777360);
  TestAlmostEqual(MatchMGRSCoords("31N BA 32007 00000"), 0.000000, 0.592330);

}

} // namespace utm_mgrs_coords_match_test
