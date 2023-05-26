#include "testing/testing.hpp"

#include "search/utm_mgrs_coords_match.hpp"

#include "base/math.hpp"

#include <iostream>

namespace utm_mgrs_coords_match_test
{
using namespace search;

// We expect the results to be quite precise.
double const kEps = 1e-5;

void TestAlmostEqual(double actual, double expected)
{
  TEST(base::AlmostEqualAbsOrRel(actual, expected, kEps), (actual, expected));
}

UNIT_TEST(MatchUTMCoords)
{
  double lat, lon;

  // Extra spaces shouldn't break format
  TEST(MatchUTMCoords("15 N  500000 4649776", lat, lon), ());
  TEST(MatchUTMCoords("15 N  500000  4649776", lat, lon), ());

  TEST(MatchUTMCoords("15N 500000 4649776", lat, lon), ());
  TestAlmostEqual(lat, 42.0);
  TestAlmostEqual(lon, -93.0);

  TEST(MatchUTMCoords("15 N 500000 4649776", lat, lon), ());
  TestAlmostEqual(lat, 42.0);
  TestAlmostEqual(lon, -93.0);

  TEST(MatchUTMCoords("32U 294409 5628898", lat, lon), ());
  TestAlmostEqual(lat, 50.77535);
  TestAlmostEqual(lon, 6.08389);

  TEST(MatchUTMCoords("32 U 294409 5628898", lat, lon), ());
  TestAlmostEqual(lat, 50.77535);
  TestAlmostEqual(lon, 6.08389);

  TEST(MatchUTMCoords("30X 476594 9328501", lat, lon), ());
  TestAlmostEqual(lat, 84.0);
  TestAlmostEqual(lon, -5.00601);

  TEST(MatchUTMCoords("30 N 476594 9328501", lat, lon), ());
  TestAlmostEqual(lat, 84.0);
  TestAlmostEqual(lon, -5.00601);
}

UNIT_TEST(MatchUTMCoords_False)
{
  double lat, lon;

  TEST(!MatchUTMCoords("2 1st", lat, lon), ());
  TEST(!MatchUTMCoords("15N5000004649776", lat, lon), ());

  // Wrong zone number (first two digits)
  TEST(!MatchUTMCoords("0X 476594 9328501", lat, lon), ());
  TEST(!MatchUTMCoords("0 X 476594 9328501", lat, lon), ());
  TEST(!MatchUTMCoords("61N 294409 5628898", lat, lon), ());
  TEST(!MatchUTMCoords("61 N 294409 5628898", lat, lon), ());

  // Wrong zone letter
  TEST(!MatchUTMCoords("25I 500000 4649776", lat, lon), ());
  TEST(!MatchUTMCoords("25 I 500000 4649776", lat, lon), ());
  TEST(!MatchUTMCoords("25O 500000 4649776", lat, lon), ());
  TEST(!MatchUTMCoords("25 O 500000 4649776", lat, lon), ());
  TEST(!MatchUTMCoords("5A 500000 4649776", lat, lon), ());
  TEST(!MatchUTMCoords("5 A 500000 4649776", lat, lon), ());
  TEST(!MatchUTMCoords("7B 500000 4649776", lat, lon), ());
  TEST(!MatchUTMCoords("7 B 500000 4649776", lat, lon), ());

  // easting out of range (must be between 100,000 m and 999,999 m)
  TEST(!MatchUTMCoords("19S 999 6360877", lat, lon), ());
  TEST(!MatchUTMCoords("19S 99999 6360877", lat, lon), ());
  TEST(!MatchUTMCoords("19S 1000000 6360877", lat, lon), ());
  TEST(!MatchUTMCoords("19S 2000000 6360877", lat, lon), ());

  // northing out of range (must be between 0 m and 10,000,000 m)
  TEST(!MatchUTMCoords("30N 476594 10000001", lat, lon), ());
  TEST(!MatchUTMCoords("30N 476594 20000000", lat, lon), ());
}

UNIT_TEST(MatchMGRSCoords_parsing)
{
  double lat, lon;

  TEST(MatchMGRSCoords("30N YF 67993 00000", lat, lon), ());
  TEST(MatchMGRSCoords("30N YF 67993 00000 ", lat, lon), ());
  TEST(MatchMGRSCoords("30N YF 67993  00000 ", lat, lon), ());
  TEST(MatchMGRSCoords("30N YF  67993  00000 ", lat, lon), ());
  TEST(MatchMGRSCoords("30NYF 67993 00000", lat, lon), ());
  TEST(MatchMGRSCoords("30NYF 67993 00000 ", lat, lon), ());
  TEST(MatchMGRSCoords("30NYF 67993  00000 ", lat, lon), ());
  TEST(MatchMGRSCoords("30NYF67993 00000", lat, lon), ());
  TEST(MatchMGRSCoords("30NYF67993 00000 ", lat, lon), ());
  TEST(MatchMGRSCoords("30NYF67993  00000 ", lat, lon), ());
  TEST(MatchMGRSCoords("30NYF6799300000", lat, lon), ());
  TEST(MatchMGRSCoords("30NYF6799300000 ", lat, lon), ());

  // Wrong number of digits
  TEST(!MatchMGRSCoords("30NYF 679930000 ", lat, lon), ());
  TEST(!MatchMGRSCoords("30NYF 679930000", lat, lon), ());

  TEST(!MatchMGRSCoords("30N YF 693 23020", lat, lon), ());
  TEST(!MatchMGRSCoords("30N YF 693 23 ", lat, lon), ());

  // Invalid zone
  TEST(!MatchMGRSCoords("30 FF 693 230", lat, lon), ());
  TEST(!MatchMGRSCoords("30A YF 693 230", lat, lon), ());
  TEST(!MatchMGRSCoords("30Z YF 693 230", lat, lon), ());
  TEST(!MatchMGRSCoords("30Z F 693 230", lat, lon), ());
  TEST(!MatchMGRSCoords("30Z 3F 693 230", lat, lon), ());
  TEST(!MatchMGRSCoords("30Z K? 693 230", lat, lon), ());
  TEST(!MatchMGRSCoords("30Z IB 693 230", lat, lon), ());
  TEST(!MatchMGRSCoords("30Z DO 693 230", lat, lon), ());

  // Wrong easting or northing
  TEST(!MatchMGRSCoords("30NYF 679_3 00000", lat, lon), ());
  TEST(!MatchMGRSCoords("30NYF 6930&000", lat, lon), ());
}

UNIT_TEST(MatchMGRSCoords_convert)
{
  double lat, lon;

  TEST(MatchMGRSCoords("30N YF 67993 00000", lat, lon), ());
  TestAlmostEqual(lat, 0.000000);
  TestAlmostEqual(lon, -0.592330);

  TEST(MatchMGRSCoords("31N BA 00000 00000", lat, lon), ());
  TestAlmostEqual(lat, 0.000000);
  TestAlmostEqual(lon, 0.304980);

  TEST(MatchMGRSCoords("32R LR 00000 00000", lat, lon), ());
  TestAlmostEqual(lat, 27.107980);
  TestAlmostEqual(lon, 6.982490);

  TEST(MatchMGRSCoords("32R MR 00000 00000", lat, lon), ());
  TestAlmostEqual(lat, 27.118850);
  TestAlmostEqual(lon, 7.991060);

  TEST(MatchMGRSCoords("33R TK 05023 99880", lat, lon), ());
  TestAlmostEqual(lat, 27.089890);
  TestAlmostEqual(lon, 12.025310);

  TEST(MatchMGRSCoords("31R GQ 69322 99158", lat, lon), ());
  TestAlmostEqual(lat, 31.596040);
  TestAlmostEqual(lon, 5.838410);

  TEST(MatchMGRSCoords("35L KF 50481 01847", lat, lon), ());
  TestAlmostEqual(lat, -13.541120);
  TestAlmostEqual(lon, 24.694510);

  TEST(MatchMGRSCoords("33L YL 59760 01153", lat, lon), ());
  TestAlmostEqual(lat, -9.028520);
  TestAlmostEqual(lon, 17.362850);

  TEST(MatchMGRSCoords("36G VR 27441 12148", lat, lon), ());
  TestAlmostEqual(lat, -45.040410);
  TestAlmostEqual(lon, 32.078730);

  TEST(MatchMGRSCoords("41R KQ 30678 99158", lat, lon), ());
  TestAlmostEqual(lat, 31.596040);
  TestAlmostEqual(lon, 60.161590);

  TEST(MatchMGRSCoords("43F CF 66291 06635", lat, lon), ());
  TestAlmostEqual(lat, -49.578080);
  TestAlmostEqual(lon, 73.150400);

  TEST(MatchMGRSCoords("43F DF 65832 14591", lat, lon), ());
  TestAlmostEqual(lat, -49.520340);
  TestAlmostEqual(lon, 74.527920);

  TEST(MatchMGRSCoords("41G NL 72559 12148", lat, lon), ());
  TestAlmostEqual(lat, -45.040410);
  TestAlmostEqual(lon, 63.921270);

  TEST(MatchMGRSCoords("41G PL 72152 04746", lat, lon), ());
  TestAlmostEqual(lat, -45.089800);
  TestAlmostEqual(lon, 65.187670);

  TEST(MatchMGRSCoords("42G UR 00000 00000", lat, lon), ());
  TestAlmostEqual(lat, -45.125150);
  TestAlmostEqual(lon, 66.456880);

  TEST(MatchMGRSCoords("42G VR 00000 00000", lat, lon), ());
  TestAlmostEqual(lat, -45.146390);
  TestAlmostEqual(lon, 67.727970);

  TEST(MatchMGRSCoords("42G WR 00000 00000", lat, lon), ());
  TestAlmostEqual(lat, -45.153480);
  TestAlmostEqual(lon, 69.000000);

  TEST(MatchMGRSCoords("42G XR 00000 00000", lat, lon), ());
  TestAlmostEqual(lat, -45.146390);
  TestAlmostEqual(lon, 70.272030);

  TEST(MatchMGRSCoords("42G YR 00000 00000", lat, lon), ());
  TestAlmostEqual(lat, -45.125150);
  TestAlmostEqual(lon, 71.543120);

  TEST(MatchMGRSCoords("43G CL 27848 04746", lat, lon), ());
  TestAlmostEqual(lat, -45.089800);
  TestAlmostEqual(lon, 72.812330);

  TEST(MatchMGRSCoords("43G DL 27441 12148", lat, lon), ());
  TestAlmostEqual(lat, -45.040410);
  TestAlmostEqual(lon, 74.078730);

  TEST(MatchMGRSCoords("41G PR 08066 09951", lat, lon), ());
  TestAlmostEqual(lat, -40.554160);
  TestAlmostEqual(lon, 64.276370);

  TEST(MatchMGRSCoords("41G QR 07714 03147", lat, lon), ());
  TestAlmostEqual(lat, -40.596410);
  TestAlmostEqual(lon, 65.454770);

  TEST(MatchMGRSCoords("42G UA 00000 00000", lat, lon), ());
  TestAlmostEqual(lat, -40.626640);
  TestAlmostEqual(lon, 66.635320);

  TEST(MatchMGRSCoords("43L GF 49519 01847", lat, lon), ());
  TestAlmostEqual(lat, -13.541120);
  TestAlmostEqual(lon, 77.305490);

  TEST(MatchMGRSCoords("45L VL 00000 00000", lat, lon), ());
  TestAlmostEqual(lat, -9.045430);
  TestAlmostEqual(lon, 86.090120);

  TEST(MatchMGRSCoords("50Q KE 64726 97325", lat, lon), ());
  TestAlmostEqual(lat, 18.051740);
  TestAlmostEqual(lon, 114.777360);

  TEST(MatchMGRSCoords("53F LF 66291 06635", lat, lon), ());
  TestAlmostEqual(lat, -49.578080);
  TestAlmostEqual(lon, 133.150400);

  TEST(MatchMGRSCoords("60F VL 65832 14591", lat, lon), ());
  TestAlmostEqual(lat, -49.520340);
  TestAlmostEqual(lon, 176.527920);


  TEST(MatchMGRSCoords("04X ER 00000 00000", lat, lon), ());
  TestAlmostEqual(lat, 81.060880);
  TestAlmostEqual(lon, -159.000000);

  TEST(MatchMGRSCoords("05X MK 95564 95053", lat, lon), ());
  TestAlmostEqual(lat, 81.016470);
  TestAlmostEqual(lon, -153.254480);

  TEST(MatchMGRSCoords("08X ME 93476 90354", lat, lon), ());
  TestAlmostEqual(lat, 72.012660);
  TestAlmostEqual(lon, -135.189280);

  TEST(MatchMGRSCoords("12H TF 59828 01847", lat, lon), ());
  TestAlmostEqual(lat, -36.098350);
  TestAlmostEqual(lon, -113.667820);

  TEST(MatchMGRSCoords("15N YA 67993 00000", lat, lon), ());
  TestAlmostEqual(lat, 0.000000);
  TestAlmostEqual(lon, -90.592330);

  TEST(MatchMGRSCoords("16N BF 00000 00000", lat, lon), ());
  TestAlmostEqual(lat, 0.000000);
  TestAlmostEqual(lon, -89.695020);

  TEST(MatchMGRSCoords("16N CF 00000 00000", lat, lon), ());
  TestAlmostEqual(lat, 0.000000);
  TestAlmostEqual(lon, -88.797050);

  TEST(MatchMGRSCoords("21L TL 40240 01153", lat, lon), ());
  TestAlmostEqual(lat, -9.028520);
  TestAlmostEqual(lon, -59.362850);

  TEST(MatchMGRSCoords("24P YV 49519 98153", lat, lon), ());
  TestAlmostEqual(lat, 13.541120);
  TestAlmostEqual(lon, -36.694510);

  TEST(MatchMGRSCoords("31K BA 64726 02675", lat, lon), ());
  TestAlmostEqual(lat, -18.051740);
  TestAlmostEqual(lon, 0.777360);

  TEST(MatchMGRSCoords("31N BA 32007 00000", lat, lon), ());
  TestAlmostEqual(lat, 0.000000);
  TestAlmostEqual(lon, 0.592330);

}

} // namespace utm_mgrs_coords_match_test
