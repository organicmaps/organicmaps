#include "../../../3party/fct/fct.h"
#include "../../src/c/api-client.h"
#include "../../internal/c/api-client-internals.h"

static const int MAX_POINT_BYTES = 10;
static const int TEST_COORD_BYTES = 9;

char const * TestLatLonToStr(double lat, double lon)
{
  static char s[TEST_COORD_BYTES + 1] = {0};
  MapsWithMe_LatLonToString(lat, lon, s, TEST_COORD_BYTES);
  return s;
}

FCT_BGN()
{

  FCT_QTEST_BGN(MapsWithMe_Base64Char)
  {
    fct_chk_eq_int('A', MapsWithMe_Base64Char(0));
    fct_chk_eq_int('B', MapsWithMe_Base64Char(1));
    fct_chk_eq_int('9', MapsWithMe_Base64Char(61));
    fct_chk_eq_int('-', MapsWithMe_Base64Char(62));
    fct_chk_eq_int('_', MapsWithMe_Base64Char(63));
  }
  FCT_QTEST_END();


  FCT_QTEST_BGN(MapsWithMe_LatToInt_0)
  {
    fct_chk_eq_int(499, MapsWithMe_LatToInt(0, 998));
    fct_chk_eq_int(500, MapsWithMe_LatToInt(0, 999));
    fct_chk_eq_int(500, MapsWithMe_LatToInt(0, 1000));
    fct_chk_eq_int(501, MapsWithMe_LatToInt(0, 1001));
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_LatToInt_NearOrGreater90)
  {
    fct_chk_eq_int(999, MapsWithMe_LatToInt(89.9, 1000));
    fct_chk_eq_int(1000, MapsWithMe_LatToInt(89.999999, 1000));
    fct_chk_eq_int(1000, MapsWithMe_LatToInt(90.0, 1000));
    fct_chk_eq_int(1000, MapsWithMe_LatToInt(90.1, 1000));
    fct_chk_eq_int(1000, MapsWithMe_LatToInt(100.0, 1000));
    fct_chk_eq_int(1000, MapsWithMe_LatToInt(180.0, 1000));
    fct_chk_eq_int(1000, MapsWithMe_LatToInt(350.0, 1000));
    fct_chk_eq_int(1000, MapsWithMe_LatToInt(360.0, 1000));
    fct_chk_eq_int(1000, MapsWithMe_LatToInt(370.0, 1000));
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_LatToInt_NearOrLess_minus90)
  {
    fct_chk_eq_int(1, MapsWithMe_LatToInt(-89.9, 1000));
    fct_chk_eq_int(0, MapsWithMe_LatToInt(-89.999999, 1000));
    fct_chk_eq_int(0, MapsWithMe_LatToInt(-90.0, 1000));
    fct_chk_eq_int(0, MapsWithMe_LatToInt(-90.1, 1000));
    fct_chk_eq_int(0, MapsWithMe_LatToInt(-100.0, 1000));
    fct_chk_eq_int(0, MapsWithMe_LatToInt(-180.0, 1000));
    fct_chk_eq_int(0, MapsWithMe_LatToInt(-350.0, 1000));
    fct_chk_eq_int(0, MapsWithMe_LatToInt(-360.0, 1000));
    fct_chk_eq_int(0, MapsWithMe_LatToInt(-370.0, 1000));
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_LatToInt_NearOrLess_Rounding)
  {
    fct_chk_eq_int(0, MapsWithMe_LatToInt(-90.0, 2));
    fct_chk_eq_int(0, MapsWithMe_LatToInt(-45.1, 2));
    fct_chk_eq_int(1, MapsWithMe_LatToInt(-45.0, 2));
    fct_chk_eq_int(1, MapsWithMe_LatToInt(0.0, 2));
    fct_chk_eq_int(1, MapsWithMe_LatToInt(44.9, 2));
    fct_chk_eq_int(2, MapsWithMe_LatToInt(45.0, 2));
    fct_chk_eq_int(2, MapsWithMe_LatToInt(90.0, 2));
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_LonIn180180)
  {
    fct_chk_eq_dbl(   0.0, MapsWithMe_LonIn180180(0));
    fct_chk_eq_dbl(  20.0, MapsWithMe_LonIn180180(20));
    fct_chk_eq_dbl(  90.0, MapsWithMe_LonIn180180(90));
    fct_chk_eq_dbl( 179.0, MapsWithMe_LonIn180180(179));
    fct_chk_eq_dbl(-180.0, MapsWithMe_LonIn180180(180));
    fct_chk_eq_dbl(-180.0, MapsWithMe_LonIn180180(-180));
    fct_chk_eq_dbl(-179.0, MapsWithMe_LonIn180180(-179));
    fct_chk_eq_dbl( -20.0, MapsWithMe_LonIn180180(-20));

    fct_chk_eq_dbl(0.0, MapsWithMe_LonIn180180(360));
    fct_chk_eq_dbl(0.0, MapsWithMe_LonIn180180(720));
    fct_chk_eq_dbl(0.0, MapsWithMe_LonIn180180(-360));
    fct_chk_eq_dbl(0.0, MapsWithMe_LonIn180180(-720));

    fct_chk_eq_dbl( 179.0, MapsWithMe_LonIn180180(360 + 179));
    fct_chk_eq_dbl(-180.0, MapsWithMe_LonIn180180(360 + 180));
    fct_chk_eq_dbl(-180.0, MapsWithMe_LonIn180180(360 - 180));
    fct_chk_eq_dbl(-179.0, MapsWithMe_LonIn180180(360 - 179));
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_LonToInt_NearOrLess_Rounding)
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

    fct_chk_eq_int(0, MapsWithMe_LonToInt(-180.0, 3));
    fct_chk_eq_int(0, MapsWithMe_LonToInt(-135.1, 3));
    fct_chk_eq_int(1, MapsWithMe_LonToInt(-135.0, 3));
    fct_chk_eq_int(1, MapsWithMe_LonToInt( -90.0, 3));
    fct_chk_eq_int(1, MapsWithMe_LonToInt( -60.1, 3));
    fct_chk_eq_int(1, MapsWithMe_LonToInt( -45.1, 3));
    fct_chk_eq_int(2, MapsWithMe_LonToInt( -45.0, 3));
    fct_chk_eq_int(2, MapsWithMe_LonToInt(   0.0, 3));
    fct_chk_eq_int(2, MapsWithMe_LonToInt(  44.9, 3));
    fct_chk_eq_int(3, MapsWithMe_LonToInt(  45.0, 3));
    fct_chk_eq_int(3, MapsWithMe_LonToInt( 120.0, 3));
    fct_chk_eq_int(3, MapsWithMe_LonToInt( 134.9, 3));
    fct_chk_eq_int(0, MapsWithMe_LonToInt( 135.0, 3));
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_LonToInt_0)
  {
    fct_chk_eq_int(499, MapsWithMe_LonToInt(0, 997));
    fct_chk_eq_int(500, MapsWithMe_LonToInt(0, 998));
    fct_chk_eq_int(500, MapsWithMe_LonToInt(0, 999));
    fct_chk_eq_int(501, MapsWithMe_LonToInt(0, 1000));
    fct_chk_eq_int(501, MapsWithMe_LonToInt(0, 1001));

    fct_chk_eq_int(499, MapsWithMe_LonToInt(360, 997));
    fct_chk_eq_int(500, MapsWithMe_LonToInt(360, 998));
    fct_chk_eq_int(500, MapsWithMe_LonToInt(360, 999));
    fct_chk_eq_int(501, MapsWithMe_LonToInt(360, 1000));
    fct_chk_eq_int(501, MapsWithMe_LonToInt(360, 1001));

    fct_chk_eq_int(499, MapsWithMe_LonToInt(-360, 997));
    fct_chk_eq_int(500, MapsWithMe_LonToInt(-360, 998));
    fct_chk_eq_int(500, MapsWithMe_LonToInt(-360, 999));
    fct_chk_eq_int(501, MapsWithMe_LonToInt(-360, 1000));
    fct_chk_eq_int(501, MapsWithMe_LonToInt(-360, 1001));
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_LonToInt_180)
  {
    fct_chk_eq_int(0, MapsWithMe_LonToInt(-180, 1000));
    fct_chk_eq_int(0, MapsWithMe_LonToInt(180, 1000));
    fct_chk_eq_int(0, MapsWithMe_LonToInt(-180 - 360, 1000));
    fct_chk_eq_int(0, MapsWithMe_LonToInt(180 + 360, 1000));
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_LonToInt_360)
  {
    fct_chk_eq_int(2, MapsWithMe_LonToInt(0, 3));
    fct_chk_eq_int(2, MapsWithMe_LonToInt(0 + 360, 3));
    fct_chk_eq_int(2, MapsWithMe_LonToInt(0 - 360, 3));

    fct_chk_eq_int(2, MapsWithMe_LonToInt(1, 3));
    fct_chk_eq_int(2, MapsWithMe_LonToInt(1 + 360, 3));
    fct_chk_eq_int(2, MapsWithMe_LonToInt(1 - 360, 3));

    fct_chk_eq_int(2, MapsWithMe_LonToInt(-1, 3));
    fct_chk_eq_int(2, MapsWithMe_LonToInt(-1 + 360, 3));
    fct_chk_eq_int(2, MapsWithMe_LonToInt(-1 - 360, 3));
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_LatLonToString)
  {
    fct_chk_eq_str("AAAAAAAAA", TestLatLonToStr(-90, -180));
    fct_chk_eq_str("qqqqqqqqq", TestLatLonToStr(90, -180));
    fct_chk_eq_str("_________", TestLatLonToStr(90, 179.999999));
    fct_chk_eq_str("VVVVVVVVV", TestLatLonToStr(-90, 179.999999));
    fct_chk_eq_str("wAAAAAAAA", TestLatLonToStr(0.0, 0.0));
    fct_chk_eq_str("6qqqqqqqq", TestLatLonToStr(90.0, 0.0));
    fct_chk_eq_str("P________", TestLatLonToStr(-0.000001, -0.000001));
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_LatLonToString_PrefixIsTheSame)
  {
    for (double lat = -95; lat <= 95; lat += 0.7)
    {
      for (double lon = -190; lon < 190; lon += 0.9)
      {
        char prevStepS[MAX_POINT_BYTES + 1] = {0};
        MapsWithMe_LatLonToString(lat, lon, prevStepS, MAX_POINT_BYTES);

        for (int len = MAX_POINT_BYTES - 1; len > 0; --len)
        {
          // Test that the current string is a prefix of the previous one.
          char s[MAX_POINT_BYTES] = {0};
          MapsWithMe_LatLonToString(lat, lon, s, len);
          prevStepS[len] = 0;
          fct_chk_eq_str(s, prevStepS);
        }
      }
    }
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_LatLonToString_StringDensity)
  {
    int b64toI[256];
    for (int i = 0; i < 256; ++i) b64toI[i] = -1;
    for (int i = 0; i < 64; ++i) b64toI[MapsWithMe_Base64Char(i)] = i;

    int num1[256] = {0};
    int num2[256][256] = {0};

    for (double lat = -90; lat <= 90; lat += 0.1)
    {
      for (double lon = -180; lon < 180; lon += 0.05)
      {
        char s[3] = {0};
        MapsWithMe_LatLonToString(lat, lon, s, 2);
        ++num1[b64toI[s[0]]];
        ++num2[b64toI[s[0]]][b64toI[s[1]]];
       }
    }

    int min1 = 1 << 30, min2 = 1 << 30;
    int max1 = 0, max2 = 0;
    for (int i = 0; i < 256; ++i)
    {
      if (num1[i] != 0 && num1[i] < min1) min1 = num1[i];
      if (num1[i] != 0 && num1[i] > max1) max1 = num1[i];
      for (int j = 0; j < 256; ++j)
      {
        if (num2[i][j] != 0 && num2[i][j] < min2) min2 = num2[i][j];
        if (num2[i][j] != 0 && num2[i][j] > max2) max2 = num2[i][j];
      }
    }

    // printf("\n1: %i-%i   2: %i-%i\n", min1, max1, min2, max2);
    fct_chk((max1 - min1) * 1.0 / max1 < 0.05);
    fct_chk((max2 - min2) * 1.0 / max2 < 0.05);
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_GenShortShowMapUrl_SmokeTest)
  {
    char buf[100] = {0};
    int res = MapsWithMe_GenShortShowMapUrl(0, 0, 19, "Name", buf, 100);
    fct_chk_eq_str("ge0://8wAAAAAAAA/Name", buf);
    fct_chk_eq_int(21, res);
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_GenShortShowMapUrl_NameIsNull)
  {
    char buf[100] = {0};
    int res = MapsWithMe_GenShortShowMapUrl(0, 0, 19, 0, buf, 100);
    fct_chk_eq_str("ge0://8wAAAAAAAA", buf);
    fct_chk_eq_int(16, res);
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_GenShortShowMapUrl_NameIsEmpty)
  {
    char buf[100] = {0};
    int res = MapsWithMe_GenShortShowMapUrl(0, 0, 19, "", buf, 100);
    fct_chk_eq_str("ge0://8wAAAAAAAA", buf);
    fct_chk_eq_int(16, res);
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_GenShortShowMapUrl_ZoomVerySmall)
  {
    char buf[100] = {0};
    int res = MapsWithMe_GenShortShowMapUrl(0, 0, 2, "Name", buf, 100);
    fct_chk_eq_str("ge0://AwAAAAAAAA/Name", buf);
    fct_chk_eq_int(21, res);
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_GenShortShowMapUrl_ZoomNegative)
  {
    char buf[100] = {0};
    int res = MapsWithMe_GenShortShowMapUrl(0, 0, -5, "Name", buf, 100);
    fct_chk_eq_str("ge0://AwAAAAAAAA/Name", buf);
    fct_chk_eq_int(21, res);
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_GenShortShowMapUrl_ZoomLarge)
  {
    char buf[100] = {0};
    int res = MapsWithMe_GenShortShowMapUrl(0, 0, 20, "Name", buf, 100);
    fct_chk_eq_str("ge0://_wAAAAAAAA/Name", buf);
    fct_chk_eq_int(21, res);
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_GenShortShowMapUrl_ZoomVeryLarge)
  {
    char buf[100] = {0};
    int res = MapsWithMe_GenShortShowMapUrl(0, 0, 2000000000, "Name", buf, 100);
    fct_chk_eq_str("ge0://_wAAAAAAAA/Name", buf);
    fct_chk_eq_int(21, res);
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_GenShortShowMapUrl_FractionalZoom)
  {
    char buf[100] = {0};
    int res = MapsWithMe_GenShortShowMapUrl(0, 0, 8.25, "Name", buf, 100);
    fct_chk_eq_str("ge0://RwAAAAAAAA/Name", buf);
    fct_chk_eq_int(21, res);
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_GenShortShowMapUrl_FractionalZoomRoundsDown)
  {
    char buf[100] = {0};
    int res = MapsWithMe_GenShortShowMapUrl(0, 0, 8.499, "Name", buf, 100);
    fct_chk_eq_str("ge0://RwAAAAAAAA/Name", buf);
    fct_chk_eq_int(21, res);
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_GenShortShowMapUrl_FractionalZoomNextStep)
  {
    char buf[100] = {0};
    int res = MapsWithMe_GenShortShowMapUrl(0, 0, 8.5, "Name", buf, 100);
    fct_chk_eq_str("ge0://SwAAAAAAAA/Name", buf);
    fct_chk_eq_int(21, res);
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_GenShortShowMapUrl_SpaceIsReplacedWithUnderscore)
  {
    char buf[100] = {0};
    int res = MapsWithMe_GenShortShowMapUrl(0, 0, 19, "Hello World", buf, 100);
    fct_chk_eq_str("ge0://8wAAAAAAAA/Hello_World", buf);
    fct_chk_eq_int(28, res);
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_GenShortShowMapUrl_NamesAreEscaped)
  {
    char buf[100] = {0};
    int res = MapsWithMe_GenShortShowMapUrl(0, 0, 19, "'Hello,World!%$", buf, 100);
    fct_chk_eq_str("ge0://8wAAAAAAAA/%27Hello%2CWorld%21%25%24", buf);
    fct_chk_eq_int(42, res);
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_GenShortShowMapUrl_UnderscoreIsReplacedWith%20)
  {
    char buf[100] = {0};
    int res = MapsWithMe_GenShortShowMapUrl(0, 0, 19, "Hello_World", buf, 100);
    fct_chk_eq_str("ge0://8wAAAAAAAA/Hello%20World", buf);
    fct_chk_eq_int(30, res);
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_GenShortShowMapUrl_ControlCharsAreEscaped)
  {
    char buf[100] = {0};
    int res = MapsWithMe_GenShortShowMapUrl(0, 0, 19, "Hello\tWorld\n", buf, 100);
    fct_chk_eq_str("ge0://8wAAAAAAAA/Hello%09World%0A", buf);
    fct_chk_eq_int(33, res);
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_GenShortShowMapUrl_BufferNullAndEmpty)
  {
    int res = MapsWithMe_GenShortShowMapUrl(0, 0, 19, "Name", 0, 0);
    fct_chk_eq_int(21, res);
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_GenShortShowMapUrl_BufferNotNullAndEmpty)
  {
    char buf[100] = {0};
    int res = MapsWithMe_GenShortShowMapUrl(0, 0, 19, "Name", buf, 0);
    fct_chk_eq_int(21, res);
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_GenShortShowMapUrl_TerminatingNullIsNotWritten)
  {
    char buf[] =   "xxxxxxxxxxxxxxxxxxxxxxxxxxx";
    int res = MapsWithMe_GenShortShowMapUrl(0, 0, 19, "Name", buf, 27);
    fct_chk_eq_str("ge0://8wAAAAAAAA/Namexxxxxx", buf);
    fct_chk_eq_int(21, res);
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_GenShortShowMapUrl_BufferIs1Byte)
  {
    char buf;
    int res = MapsWithMe_GenShortShowMapUrl(0, 0, 19, "Name", &buf, 1);
    fct_chk_eq_int('g', buf);
    fct_chk_eq_int(21, res);
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_GenShortShowMapUrl_BufferTooSmall)
  {
    for (int bufSize = 1; bufSize <= 21; ++bufSize)
    {
      char buf[100] = {0};
      int res = MapsWithMe_GenShortShowMapUrl(0, 0, 19, "Name", buf, bufSize);
      char expected[] = "ge0://8wAAAAAAAA/Name";
      expected[bufSize] = 0;
      fct_chk_eq_str(expected, buf);
      fct_chk_eq_int(21, res);
    }
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_GenShortShowMapUrl_Unicode)
  {
    char buf[100] = {0};
    int res = MapsWithMe_GenShortShowMapUrl(0, 0, 19, "\xe2\x98\x84", buf, 100);
    fct_chk_eq_str("ge0://8wAAAAAAAA/\xe2\x98\x84", buf);
    fct_chk_eq_int(20, res);
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_GenShortShowMapUrl_UnicodeMixedWithOtherChars)
  {
    char buf[100] = {0};
    int res = MapsWithMe_GenShortShowMapUrl(0, 0, 19, "Back_in \xe2\x98\x84!\xd1\x8e\xd0\xbc", buf, 100);
    fct_chk_eq_str("ge0://8wAAAAAAAA/Back%20in_\xe2\x98\x84%21\xd1\x8e\xd0\xbc", buf);
    fct_chk_eq_int(37, res);
  }
  FCT_QTEST_END();

}
FCT_END();
