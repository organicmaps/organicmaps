#include "testing/testing.hpp"

#include "ge0/url_generator.hpp"

#include <string>

using namespace std;

namespace
{
int const kTestCoordBytes = 9;
}  // namespace

namespace ge0
{
string TestLatLonToStr(double lat, double lon)
{
  static char s[kTestCoordBytes + 1] = {0};
  LatLonToString(lat, lon, s, kTestCoordBytes);
  return string(s);
}

UNIT_TEST(Base64Char)
{
  TEST_EQUAL('A', Base64Char(0), ());
  TEST_EQUAL('B', Base64Char(1), ());
  TEST_EQUAL('9', Base64Char(61), ());
  TEST_EQUAL('-', Base64Char(62), ());
  TEST_EQUAL('_', Base64Char(63), ());
}

UNIT_TEST(LatToInt_0)
{
  TEST_EQUAL(499, LatToInt(0, 998), ());
  TEST_EQUAL(500, LatToInt(0, 999), ());
  TEST_EQUAL(500, LatToInt(0, 1000), ());
  TEST_EQUAL(501, LatToInt(0, 1001), ());
}

UNIT_TEST(LatToInt_NearOrGreater90)
{
  TEST_EQUAL(999, LatToInt(89.9, 1000), ());
  TEST_EQUAL(1000, LatToInt(89.999999, 1000), ());
  TEST_EQUAL(1000, LatToInt(90.0, 1000), ());
  TEST_EQUAL(1000, LatToInt(90.1, 1000), ());
  TEST_EQUAL(1000, LatToInt(100.0, 1000), ());
  TEST_EQUAL(1000, LatToInt(180.0, 1000), ());
  TEST_EQUAL(1000, LatToInt(350.0, 1000), ());
  TEST_EQUAL(1000, LatToInt(360.0, 1000), ());
  TEST_EQUAL(1000, LatToInt(370.0, 1000), ());
}

UNIT_TEST(LatToInt_NearOrLess_minus90)
{
  TEST_EQUAL(1, LatToInt(-89.9, 1000), ());
  TEST_EQUAL(0, LatToInt(-89.999999, 1000), ());
  TEST_EQUAL(0, LatToInt(-90.0, 1000), ());
  TEST_EQUAL(0, LatToInt(-90.1, 1000), ());
  TEST_EQUAL(0, LatToInt(-100.0, 1000), ());
  TEST_EQUAL(0, LatToInt(-180.0, 1000), ());
  TEST_EQUAL(0, LatToInt(-350.0, 1000), ());
  TEST_EQUAL(0, LatToInt(-360.0, 1000), ());
  TEST_EQUAL(0, LatToInt(-370.0, 1000), ());
}

UNIT_TEST(LatToInt_NearOrLess_Rounding)
{
  TEST_EQUAL(0, LatToInt(-90.0, 2), ());
  TEST_EQUAL(0, LatToInt(-45.1, 2), ());
  TEST_EQUAL(1, LatToInt(-45.0, 2), ());
  TEST_EQUAL(1, LatToInt(0.0, 2), ());
  TEST_EQUAL(1, LatToInt(44.9, 2), ());
  TEST_EQUAL(2, LatToInt(45.0, 2), ());
  TEST_EQUAL(2, LatToInt(90.0, 2), ());
}

UNIT_TEST(LonIn180180)
{
  double const kEps = 1e-20;

  TEST_ALMOST_EQUAL_ABS(0.0, LonIn180180(0), kEps, ());
  TEST_ALMOST_EQUAL_ABS(20.0, LonIn180180(20), kEps, ());
  TEST_ALMOST_EQUAL_ABS(90.0, LonIn180180(90), kEps, ());
  TEST_ALMOST_EQUAL_ABS(179.0, LonIn180180(179), kEps, ());
  TEST_ALMOST_EQUAL_ABS(-180.0, LonIn180180(180), kEps, ());
  TEST_ALMOST_EQUAL_ABS(-180.0, LonIn180180(-180), kEps, ());
  TEST_ALMOST_EQUAL_ABS(-179.0, LonIn180180(-179), kEps, ());
  TEST_ALMOST_EQUAL_ABS(-20.0, LonIn180180(-20), kEps, ());

  TEST_ALMOST_EQUAL_ABS(0.0, LonIn180180(360), kEps, ());
  TEST_ALMOST_EQUAL_ABS(0.0, LonIn180180(720), kEps, ());
  TEST_ALMOST_EQUAL_ABS(0.0, LonIn180180(-360), kEps, ());
  TEST_ALMOST_EQUAL_ABS(0.0, LonIn180180(-720), kEps, ());

  TEST_ALMOST_EQUAL_ABS(179.0, LonIn180180(360 + 179), kEps, ());
  TEST_ALMOST_EQUAL_ABS(-180.0, LonIn180180(360 + 180), kEps, ());
  TEST_ALMOST_EQUAL_ABS(-180.0, LonIn180180(360 - 180), kEps, ());
  TEST_ALMOST_EQUAL_ABS(-179.0, LonIn180180(360 - 179), kEps, ());
}

UNIT_TEST(LonToInt_NearOrLess_Rounding)
{
  //     135   90  45
  //        \  |  /
  //         03333
  //  180    0\|/2
  //    -----0-o-2---- 0
  // -180    0/|\2
  //         11112
  //        /  |  \
    //    -135  -90 -45

  TEST_EQUAL(0, LonToInt(-180.0, 3), ());
  TEST_EQUAL(0, LonToInt(-135.1, 3), ());
  TEST_EQUAL(1, LonToInt(-135.0, 3), ());
  TEST_EQUAL(1, LonToInt(-90.0, 3), ());
  TEST_EQUAL(1, LonToInt(-60.1, 3), ());
  TEST_EQUAL(1, LonToInt(-45.1, 3), ());
  TEST_EQUAL(2, LonToInt(-45.0, 3), ());
  TEST_EQUAL(2, LonToInt(0.0, 3), ());
  TEST_EQUAL(2, LonToInt(44.9, 3), ());
  TEST_EQUAL(3, LonToInt(45.0, 3), ());
  TEST_EQUAL(3, LonToInt(120.0, 3), ());
  TEST_EQUAL(3, LonToInt(134.9, 3), ());
  TEST_EQUAL(0, LonToInt(135.0, 3), ());
}

UNIT_TEST(LonToInt_0)
{
  TEST_EQUAL(499, LonToInt(0, 997), ());
  TEST_EQUAL(500, LonToInt(0, 998), ());
  TEST_EQUAL(500, LonToInt(0, 999), ());
  TEST_EQUAL(501, LonToInt(0, 1000), ());
  TEST_EQUAL(501, LonToInt(0, 1001), ());

  TEST_EQUAL(499, LonToInt(360, 997), ());
  TEST_EQUAL(500, LonToInt(360, 998), ());
  TEST_EQUAL(500, LonToInt(360, 999), ());
  TEST_EQUAL(501, LonToInt(360, 1000), ());
  TEST_EQUAL(501, LonToInt(360, 1001), ());

  TEST_EQUAL(499, LonToInt(-360, 997), ());
  TEST_EQUAL(500, LonToInt(-360, 998), ());
  TEST_EQUAL(500, LonToInt(-360, 999), ());
  TEST_EQUAL(501, LonToInt(-360, 1000), ());
  TEST_EQUAL(501, LonToInt(-360, 1001), ());
}

UNIT_TEST(LonToInt_180)
{
  TEST_EQUAL(0, LonToInt(-180, 1000), ());
  TEST_EQUAL(0, LonToInt(180, 1000), ());
  TEST_EQUAL(0, LonToInt(-180 - 360, 1000), ());
  TEST_EQUAL(0, LonToInt(180 + 360, 1000), ());
}

UNIT_TEST(LonToInt_360)
{
  TEST_EQUAL(2, LonToInt(0, 3), ());
  TEST_EQUAL(2, LonToInt(0 + 360, 3), ());
  TEST_EQUAL(2, LonToInt(0 - 360, 3), ());

  TEST_EQUAL(2, LonToInt(1, 3), ());
  TEST_EQUAL(2, LonToInt(1 + 360, 3), ());
  TEST_EQUAL(2, LonToInt(1 - 360, 3), ());

  TEST_EQUAL(2, LonToInt(-1, 3), ());
  TEST_EQUAL(2, LonToInt(-1 + 360, 3), ());
  TEST_EQUAL(2, LonToInt(-1 - 360, 3), ());
}

UNIT_TEST(LatLonToString)
{
  TEST_EQUAL("AAAAAAAAA", TestLatLonToStr(-90, -180), ());
  TEST_EQUAL("qqqqqqqqq", TestLatLonToStr(90, -180), ());
  TEST_EQUAL("_________", TestLatLonToStr(90, 179.999999), ());
  TEST_EQUAL("VVVVVVVVV", TestLatLonToStr(-90, 179.999999), ());
  TEST_EQUAL("wAAAAAAAA", TestLatLonToStr(0.0, 0.0), ());
  TEST_EQUAL("6qqqqqqqq", TestLatLonToStr(90.0, 0.0), ());
  TEST_EQUAL("P________", TestLatLonToStr(-0.000001, -0.000001), ());
}

UNIT_TEST(LatLonToString_PrefixIsTheSame)
{
  for (double lat = -95; lat <= 95; lat += 0.7)
  {
    for (double lon = -190; lon < 190; lon += 0.9)
    {
      char prevStepS[kMaxPointBytes + 1] = {0};
      LatLonToString(lat, lon, prevStepS, kMaxPointBytes);

      for (int len = kMaxPointBytes - 1; len > 0; --len)
      {
        // Test that the current string is a prefix of the previous one.
        char s[kMaxPointBytes] = {0};
        LatLonToString(lat, lon, s, len);
        prevStepS[len] = 0;
        TEST_EQUAL(s, string(prevStepS), ());
      }
    }
  }
}

UNIT_TEST(LatLonToString_StringDensity)
{
  int b64toI[256];
  for (int i = 0; i < 256; ++i)
    b64toI[i] = -1;
  for (int i = 0; i < 64; ++i)
    b64toI[static_cast<size_t>(Base64Char(i))] = i;

  int num1[256] = {0};
  int num2[256][256] = {{0}};

  for (double lat = -90; lat <= 90; lat += 0.1)
  {
    for (double lon = -180; lon < 180; lon += 0.05)
    {
      char s[3] = {0};
      LatLonToString(lat, lon, s, 2);
      auto const s0 = static_cast<size_t>(s[0]);
      auto const s1 = static_cast<size_t>(s[1]);
      ++num1[b64toI[s0]];
      ++num2[b64toI[s0]][b64toI[s1]];
    }
  }

  int min1 = 1 << 30, min2 = 1 << 30;
  int max1 = 0, max2 = 0;
  for (int i = 0; i < 256; ++i)
  {
    if (num1[i] != 0 && num1[i] < min1)
      min1 = num1[i];
    if (num1[i] != 0 && num1[i] > max1)
      max1 = num1[i];
    for (int j = 0; j < 256; ++j)
    {
      if (num2[i][j] != 0 && num2[i][j] < min2)
        min2 = num2[i][j];
      if (num2[i][j] != 0 && num2[i][j] > max2)
        max2 = num2[i][j];
    }
  }

  // printf("\n1: %i-%i   2: %i-%i\n", min1, max1, min2, max2);
  TEST((max1 - min1) * 1.0 / max1 < 0.05, ());
  TEST((max2 - min2) * 1.0 / max2 < 0.05, ());
}

UNIT_TEST(GenShortShowMapUrl_SmokeTest)
{
  char buf[100] = {0};
  int res = GenShortShowMapUrl(0, 0, 19, "Name", buf, 100);
  TEST_EQUAL("ge0://8wAAAAAAAA/Name", string(buf), ());
  TEST_EQUAL(21, res, ());
}

UNIT_TEST(GenShortShowMapUrl_NameIsNull)
{
  char buf[100] = {0};
  int res = GenShortShowMapUrl(0, 0, 19, 0, buf, 100);
  TEST_EQUAL("ge0://8wAAAAAAAA", string(buf), ());
  TEST_EQUAL(16, res, ());
}

UNIT_TEST(GenShortShowMapUrl_NameIsEmpty)
{
  char buf[100] = {0};
  int res = GenShortShowMapUrl(0, 0, 19, "", buf, 100);
  TEST_EQUAL("ge0://8wAAAAAAAA", string(buf), ());
  TEST_EQUAL(16, res, ());
}

UNIT_TEST(GenShortShowMapUrl_ZoomVerySmall)
{
  char buf[100] = {0};
  int res = GenShortShowMapUrl(0, 0, 2, "Name", buf, 100);
  TEST_EQUAL("ge0://AwAAAAAAAA/Name", string(buf), ());
  TEST_EQUAL(21, res, ());
}

UNIT_TEST(GenShortShowMapUrl_ZoomNegative)
{
  char buf[100] = {0};
  int res = GenShortShowMapUrl(0, 0, -5, "Name", buf, 100);
  TEST_EQUAL("ge0://AwAAAAAAAA/Name", string(buf), ());
  TEST_EQUAL(21, res, ());
}

UNIT_TEST(GenShortShowMapUrl_ZoomLarge)
{
  char buf[100] = {0};
  int res = GenShortShowMapUrl(0, 0, 20, "Name", buf, 100);
  TEST_EQUAL("ge0://_wAAAAAAAA/Name", string(buf), ());
  TEST_EQUAL(21, res, ());
}

UNIT_TEST(GenShortShowMapUrl_ZoomVeryLarge)
{
  char buf[100] = {0};
  int res = GenShortShowMapUrl(0, 0, 2000000000, "Name", buf, 100);
  TEST_EQUAL("ge0://_wAAAAAAAA/Name", string(buf), ());
  TEST_EQUAL(21, res, ());
}

UNIT_TEST(GenShortShowMapUrl_FractionalZoom)
{
  char buf[100] = {0};
  int res = GenShortShowMapUrl(0, 0, 8.25, "Name", buf, 100);
  TEST_EQUAL("ge0://RwAAAAAAAA/Name", string(buf), ());
  TEST_EQUAL(21, res, ());
}

UNIT_TEST(GenShortShowMapUrl_FractionalZoomRoundsDown)
{
  char buf[100] = {0};
  int res = GenShortShowMapUrl(0, 0, 8.499, "Name", buf, 100);
  TEST_EQUAL("ge0://RwAAAAAAAA/Name", string(buf), ());
  TEST_EQUAL(21, res, ());
}

UNIT_TEST(GenShortShowMapUrl_FractionalZoomNextStep)
{
  char buf[100] = {0};
  int res = GenShortShowMapUrl(0, 0, 8.5, "Name", buf, 100);
  TEST_EQUAL("ge0://SwAAAAAAAA/Name", string(buf), ());
  TEST_EQUAL(21, res, ());
}

UNIT_TEST(GenShortShowMapUrl_SpaceIsReplacedWithUnderscore)
{
  char buf[100] = {0};
  int res = GenShortShowMapUrl(0, 0, 19, "Hello World", buf, 100);
  TEST_EQUAL("ge0://8wAAAAAAAA/Hello_World", string(buf), ());
  TEST_EQUAL(28, res, ());
}

UNIT_TEST(GenShortShowMapUrl_NamesAreEscaped)
{
  char buf[100] = {0};
  int res = GenShortShowMapUrl(0, 0, 19, "'Hello,World!%$", buf, 100);
  TEST_EQUAL("ge0://8wAAAAAAAA/%27Hello%2CWorld%21%25%24", string(buf), ());
  TEST_EQUAL(42, res, ());
}

UNIT_TEST(GenShortShowMapUrl_UnderscoreIsReplacedWith_Percent_20)
{
  char buf[100] = {0};
  int res = GenShortShowMapUrl(0, 0, 19, "Hello_World", buf, 100);
  TEST_EQUAL("ge0://8wAAAAAAAA/Hello%20World", string(buf), ());
  TEST_EQUAL(30, res, ());
}

UNIT_TEST(GenShortShowMapUrl_ControlCharsAreEscaped)
{
  char buf[100] = {0};
  int res = GenShortShowMapUrl(0, 0, 19, "Hello\tWorld\n", buf, 100);
  TEST_EQUAL("ge0://8wAAAAAAAA/Hello%09World%0A", string(buf), ());
  TEST_EQUAL(33, res, ());
}

UNIT_TEST(GenShortShowMapUrl_BufferNullAndEmpty)
{
  int res = GenShortShowMapUrl(0, 0, 19, "Name", nullptr, 0);
  TEST_EQUAL(21, res, ());
}

UNIT_TEST(GenShortShowMapUrl_BufferNotNullAndEmpty)
{
  char buf[100] = {0};
  int res = GenShortShowMapUrl(0, 0, 19, "Name", buf, 0);
  TEST_EQUAL(21, res, ());
}

UNIT_TEST(GenShortShowMapUrl_TerminatingNullIsNotWritten)
{
  char buf[] = "xxxxxxxxxxxxxxxxxxxxxxxxxxx";
  int res = GenShortShowMapUrl(0, 0, 19, "Name", buf, 27);
  TEST_EQUAL("ge0://8wAAAAAAAA/Namexxxxxx", string(buf), ());
  TEST_EQUAL(21, res, ());
}

UNIT_TEST(GenShortShowMapUrl_BufferIs1Byte)
{
  char buf;
  int res = GenShortShowMapUrl(0, 0, 19, "Name", &buf, 1);
  TEST_EQUAL('g', buf, ());
  TEST_EQUAL(21, res, ());
}

UNIT_TEST(GenShortShowMapUrl_BufferTooSmall)
{
  for (int bufSize = 1; bufSize <= 21; ++bufSize)
  {
    char buf[100] = {0};
    int res = GenShortShowMapUrl(0, 0, 19, "Name", buf, bufSize);
    char expected[] = "ge0://8wAAAAAAAA/Name";
    expected[bufSize] = 0;
    TEST_EQUAL(string(expected), string(buf), ());
    TEST_EQUAL(21, res, ());
  }
}

UNIT_TEST(GenShortShowMapUrl_Unicode)
{
  char buf[100] = {0};
  int res = GenShortShowMapUrl(0, 0, 19, "\xe2\x98\x84", buf, 100);
  TEST_EQUAL("ge0://8wAAAAAAAA/\xe2\x98\x84", string(buf), ());
  TEST_EQUAL(20, res, ());
}

UNIT_TEST(GenShortShowMapUrl_UnicodeMixedWithOtherChars)
{
  char buf[100] = {0};
  int res = GenShortShowMapUrl(0, 0, 19, "Back_in \xe2\x98\x84!\xd1\x8e\xd0\xbc", buf, 100);
  TEST_EQUAL("ge0://8wAAAAAAAA/Back%20in_\xe2\x98\x84%21\xd1\x8e\xd0\xbc", string(buf), ());
  TEST_EQUAL(37, res, ());
}
}  // namespace ge0
