#include "testing/testing.hpp"

#include "base/sunrise_sunset.hpp"

#include "base/timegm.hpp"

// Test site for sunrise and sunset is
// http://voshod-solnca.ru/

UNIT_TEST(SunriseSunsetAlgorithm_Moscow_April)
{
  // Moscow (utc +3), date 2015/4/12:
  // Sunrise utc time: 2015/4/12,2:34
  // Sunset utc time: 2015/4/12,16:29
  double const lat = 55.7522222;
  double const lon = 37.6155556;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 4, 12, 2, 10, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 4, 12, 2, 45, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 4, 12, 16, 15, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 4, 12, 16, 45, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Moscow_July)
{
  // Moscow (utc +3), date 2015/7/13:
  // Sunrise utc time: 2015/7/13,1:04
  // Sunset utc time: 2015/7/13,18:09
  double const lat = 55.7522222;
  double const lon = 37.6155556;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 7, 13, 0, 50, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 7, 13, 1, 45, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 7, 13, 18, 0, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 7, 13, 18, 30, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Moscow_September)
{
  // Moscow (utc +3), date 2015/9/17:
  // Sunrise utc time: 2015/9/17,3:05
  // Sunset utc time: 2015/9/17,15:46
  double const lat = 55.7522222;
  double const lon = 37.6155556;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 9, 17, 2, 55, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 9, 17, 3, 15, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 9, 17, 15, 0, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 9, 17, 16, 0, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Moscow_December)
{
  // Moscow (utc +3), date 2015/12/25:
  // Sunrise utc time: 2015/12/25,06:00
  // Sunset utc time: 2015/12/25,13:01
  double const lat = 55.7522222;
  double const lon = 37.6155556;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 12, 25, 5, 55, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 12, 25, 6, 10, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 12, 25, 12, 55, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 12, 25, 13, 10, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Moscow_NewYear)
{
  // Moscow (utc +3), date 2016/1/1:
  // Sunrise utc time: 2016/1/1,6:1
  // Sunset utc time: 2016/1/1,13:7
  double const lat = 55.7522222;
  double const lon = 37.6155556;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2016, 1, 1, 5, 55, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2016, 1, 1, 6, 10, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2016, 1, 1, 13, 0, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2016, 1, 1, 13, 15, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Paris_NewYear)
{
  // Paris (utc +1)
  // Sunrise utc time: 2016/1/1,7:45
  // Sunset utc time: 2016/1/1,16:04
  double const lat = 48.875649;
  double const lon = 2.344428;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2016, 1, 1, 7, 35, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2016, 1, 1, 7, 55, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2016, 1, 1, 16, 0, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2016, 1, 1, 16, 10, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Honolulu_February)
{
  // Honolulu (utc -10), date 2015/2/12:
  // Sunrise utc time: 2015/2/12,17:05
  // Sunset utc time: 2015/2/13,4:29
  double const lat = 21.307431;
  double const lon = -157.848568;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 2, 12, 17, 0, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 2, 12, 17, 15, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 2, 13, 4, 10, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 2, 13, 4, 35, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Honolulu_July)
{
  // Honolulu (utc -10). For date 2015/7/13:
  // Sunrise utc time: 2015/7/13,15:58
  // Sunset utc time: 2015/7/14,5:18
  double const lat = 21.307431;
  double const lon = -157.848568;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 7, 13, 15, 40, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 7, 13, 16, 10, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 7, 14, 5, 5, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 7, 14, 5, 25, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Honolulu_June)
{
  // Honolulu (utc -10). For date 2015/6/22:
  // Sunrise utc time: 2015/6/22,15:51
  // Sunset utc time: 2015/6/23,5:17
  double const lat = 21.307431;
  double const lon = -157.848568;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 6, 22, 15, 40, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 6, 22, 16, 0, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 6, 23, 5, 5, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 6, 23, 5, 25, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Honolulu_December)
{
  // Honolulu (utc -10). For date 2015/12/23:
  // Sunrise utc time: 2015/12/23,17:06
  // Sunset utc time: 2015/12/24,3:56
  double const lat = 21.307431;
  double const lon = -157.848568;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 12, 23, 16, 40, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 12, 23, 17, 10, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 12, 23, 3, 50, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 12, 23, 4, 5, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Melbourne_Ferbuary)
{
  // Melbourne (utc +11). For date 2015/2/12:
  // Sunrise utc time: 2015/2/11,19:46
  // Sunset utc time: 2015/2/12,9:24
  double const lat = -37.829188;
  double const lon = 144.957976;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 2, 11, 19, 30, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 2, 11, 19, 50, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 2, 12, 9, 15, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 2, 12, 9, 30, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Melbourne_NewYear)
{
  // Melbourne (utc +11). For date 2016/1/1:
  // Sunrise utc time: 2015/12/31,19:02
  // Sunset utc time: 2016/1/1,9:46
  double const lat = -37.829188;
  double const lon = 144.957976;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 12, 31, 18, 55, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 12, 31, 19, 5, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2016, 1, 1, 9, 40, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2016, 1, 1, 9, 55, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_GetDayTime_Melbourne_August)
{
  // Melbourne (utc +11), 2015/8/12
  // prev sunset utc 2015/8/11,7:41
  // sunrise utc 2015/8/11,21:10
  // sunset utc 2015/8/12,7:42
  // next sunrise utc 2015/8/12,21:09
  double const lat = -37.829188;
  double const lon = 144.957976;

  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 8, 11, 7, 30, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 8, 11, 7, 50, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 8, 11, 21, 0, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 8, 11, 21, 20, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 8, 12, 7, 35, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 8, 12, 7, 55, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 8, 12, 21, 0, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 8, 12, 21, 15, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Wellington_October)
{
  // Melbourne (utc +13). For date 2015/10/20:
  // Sunrise utc time: 2015/10/19,17:26
  // Sunset utc time: 2015/10/20,6:47
  double const lat = -41.287481;
  double const lon = 174.774189;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 10, 19, 17, 15, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 10, 19, 17, 35, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 10, 20, 6, 40, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 10, 20, 6, 55, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_BuenosAires_March)
{
  // Buenos Aires (utc -3). For date 2015/3/8:
  // Sunrise utc time: 2015/3/8,9:48
  // Sunset utc time: 2015/3/8,22:23
  double const lat = -34.607639;
  double const lon = -58.438095;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 3, 8, 9, 40, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 3, 8, 10, 5, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 3, 8, 22, 20, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 3, 8, 22, 28, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Seattle_May)
{
  // Seattle (utc -8). For date 2015/5/9:
  // Sunrise utc time: 2015/5/9,12:41
  // Sunset utc time: 2015/5/10,3:32
  double const lat = 47.597482;
  double const lon = -122.334590;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 5, 9, 12, 35, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 5, 9, 12, 45, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 5, 10, 3, 20, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 5, 10, 3, 40, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Reykjavik_May)
{
  // Reykjavik (utc 0). For date 2015/5/9:
  // Sunrise utc time: 2015/5/9,4:34
  // Sunset utc time: 2015/5/9,22:15
  double const lat = 64.120467;
  double const lon = -21.809448;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 5, 9, 4, 30, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 5, 9, 4, 40, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 5, 9, 22, 10, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 5, 9, 22, 20, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Reykjavik_June)
{
  // Reykjavik (utc 0). For date 2015/6/22:
  // Sunrise utc time: 2015/6/22,2:56
  // Sunset utc time: 2015/6/23,0:04
  double const lat = 64.120467;
  double const lon = -21.809448;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 6, 22, 2, 45, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 6, 22, 3, 5, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 6, 23, 0, 0, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 6, 23, 0, 7, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_CapeTown_November)
{
  // Cape Town (utc +2). For date 2015/11/11:
  // Sunrise utc time: 2015/11/11,3:38
  // Sunset utc time: 2015/11/11,17:24
  double const lat = -33.929573;
  double const lon = 18.428439;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 11, 11, 3, 30, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 11, 11, 3, 45, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 11, 11, 17, 20, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 11, 11, 17, 30, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_CapeTown_March)
{
  // Cape Town (utc +2). For date 2015/3/1:
  // Sunrise utc time: 2015/3/1,4:34
  // Sunset utc time: 2015/3/1,17:24
  double const lat = -33.929573;
  double const lon = 18.428439;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 3, 1, 4, 30, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 3, 1, 4, 40, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 3, 1, 17, 20, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 3, 1, 17, 30, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Tiksi_March)
{
  // Russia, Siberia, Tiksi. For date 2015/3/1:
  // Sunrise utc time: 2015/2/28,23:04
  // Sunset utc time: 2015/3/1,8:12
  double const lat = 71.635604;
  double const lon = 128.882922;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 2, 28, 23, 0, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 2, 28, 23, 10, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 3, 1, 8, 10, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 3, 1, 8, 15, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Tiksi_July)
{
  // Russia, Siberia, Tiksi. For date 2015/7/1:
  // Polar day
  double const lat = 71.635604;
  double const lon = 128.882922;

  TEST_EQUAL(DayTimeType::PolarDay, GetDayTime(base::TimeGM(2015, 7, 1, 0, 0, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::PolarDay, GetDayTime(base::TimeGM(2015, 7, 1, 12, 0, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::PolarDay, GetDayTime(base::TimeGM(2015, 7, 1, 23, 0, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Tiksi_December)
{
  // Russia, Siberia, Tiksi. For date 2015/12/1:
  // Polar night
  double const lat = 71.635604;
  double const lon = 128.882922;

  TEST_EQUAL(DayTimeType::PolarNight, GetDayTime(base::TimeGM(2015, 12, 1, 0, 0, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::PolarNight, GetDayTime(base::TimeGM(2015, 12, 1, 12, 0, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::PolarNight, GetDayTime(base::TimeGM(2015, 12, 1, 23, 0, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Norilsk_NewYear)
{
  // Norilsk. For date 2016/1/1:
  // Polar night
  double const lat = 69.350000;
  double const lon = 88.180000;

  TEST_EQUAL(DayTimeType::PolarNight, GetDayTime(base::TimeGM(2016, 1, 1, 0, 0, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::PolarNight, GetDayTime(base::TimeGM(2016, 1, 1, 12, 0, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::PolarNight, GetDayTime(base::TimeGM(2016, 1, 1, 23, 0, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Norilsk_August)
{
  // Norilsk. For date 2015/6/22:
  // Polar day
  double const lat = 69.350000;
  double const lon = 88.180000;

  TEST_EQUAL(DayTimeType::PolarDay, GetDayTime(base::TimeGM(2015, 6, 22, 0, 0, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::PolarDay, GetDayTime(base::TimeGM(2015, 6, 22, 12, 0, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::PolarDay, GetDayTime(base::TimeGM(2015, 6, 22, 23, 0, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Tokio_September)
{
  // Tokio. For date 2015/9/12:
  // Sunrise utc time: 2015/9/11,20:22
  // Sunset utc time: 2015/9/12,8:56
  double const lat = 35.715791;
  double const lon = 139.743945;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 9, 12, 20, 20, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 9, 12, 20, 25, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 9, 12, 8, 50, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 9, 12, 9, 0, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Kabul_March)
{
  // Kabul. For date 2015/3/20:
  // Sunrise utc time: 2015/3/20,01:29
  // Sunset utc time: 2015/3/20,13:35
  double const lat = 34.552312;
  double const lon = 69.170520;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 3, 20, 1, 25, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 3, 20, 1, 35, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 3, 20, 13, 30, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 3, 20, 13, 40, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Areora_January)
{
  // Areora. For date 2016/1/1:
  // Sunrise utc time: 2016/1/1,15:57
  // Sunset utc time: 2016/1/2,5:16
  double const lat = -20.003751;
  double const lon = -158.114640;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2016, 1, 1, 15, 50, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2016, 1, 1, 16, 5, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2016, 1, 2, 5, 10, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2016, 1, 2, 5, 20, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Lorino_February)
{
  // Lorino (utc +12). For date 2016/2/2:
  // Sunrise utc time: 2016/2/2,20:17
  // Sunset utc time: 2016/2/3,3:10
  double const lat = 65.499550;
  double const lon = -171.715726;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2016, 2, 2, 20, 10, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2016, 2, 2, 20, 20, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2016, 2, 3, 3, 0, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2016, 2, 3, 3, 20, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Anadyr_December)
{
  // Anadyr. For date 2015/12/25:
  // Sunrise utc time: 2015/12/24,22:17
  // Sunset utc time: 2015/12/25,02:03
  double const lat = 64.722245;
  double const lon = 177.499123;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 12, 24, 22, 10, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 12, 24, 22, 20, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 12, 25, 2, 0, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 12, 25, 2, 5, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Nikolski_December)
{
  // Nikolski. For date 2015/12/25:
  // Sunrise utc time: 2015/12/25,19:29
  // Sunset utc time: 2015/12/26,3:04
  double const lat = 52.933280;
  double const lon = -168.864102;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 12, 25, 19, 0, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 12, 25, 19, 35, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 12, 26, 3, 0, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 12, 26, 3, 10, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Kiribati_July)
{
  // Kiribati. For date 2015/7/1:
  // Sunrise utc time: 2015/7/1,16:28
  // Sunset utc time: 2015/7/2,4:41
  double const lat = 1.928797;
  double const lon = -157.494678;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 7, 1, 16, 10, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 7, 1, 16, 35, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 7, 2, 4, 0, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 7, 2, 4, 50, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Kiribati_July_2)
{
  // Kiribati. For date 2015/7/1:
  // Sunrise utc time: 2015/7/1,16:28
  // Sunset utc time: 2015/7/2,4:41
  // Next sunrise utc time: 2015/7/2,16:28
  // Next sunset utc time: 2015/7/3,4:42
  double const lat = 1.928797;
  double const lon = -157.494678;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 7, 1, 16, 20, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 7, 1, 16, 35, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 7, 2, 4, 35, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 7, 2, 4, 50, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 7, 2, 16, 20, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 7, 2, 16, 35, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 7, 3, 4, 35, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 7, 3, 4, 50, 0), lat, lon), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_London_July)
{
  // London. For date 2015/7/1:
  // Sunrise utc time: 2015/7/1,3:47
  // Sunset utc time: 2015/7/1,20:21
  double const lat = 51.500000;
  double const lon = 0.120000;

  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 7, 1, 2, 50, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 7, 1, 16, 20, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Day, GetDayTime(base::TimeGM(2015, 7, 1, 20, 10, 0), lat, lon), ());
  TEST_EQUAL(DayTimeType::Night, GetDayTime(base::TimeGM(2015, 7, 1, 21, 15, 0), lat, lon), ());
}
