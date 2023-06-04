#include "testing/testing.hpp"

#include <ctime>

#include "base/timegm.hpp"

UNIT_TEST(TimegmTest)
{
  std::tm tm1{};
  std::tm tm2{};

  TEST(strptime("2016-05-17 07:10", "%Y-%m-%d %H:%M", &tm1), ());
  TEST(strptime("2016-05-17 07:10", "%Y-%m-%d %H:%M", &tm2), ());
  TEST_EQUAL(timegm(&tm1), base::TimeGM(tm2), ());
  TEST_EQUAL(timegm(&tm1), base::TimeGM(2016, 5, 17, 7, 10, 0), ());

  TEST(strptime("2016-03-12 11:10", "%Y-%m-%d %H:%M", &tm1), ());
  TEST(strptime("2016-03-12 11:10", "%Y-%m-%d %H:%M", &tm2), ());
  TEST_EQUAL(timegm(&tm1), base::TimeGM(tm2), ());
  TEST_EQUAL(timegm(&tm1), base::TimeGM(2016, 3, 12, 11, 10, 0), ());

  TEST(strptime("1970-01-01 00:00", "%Y-%m-%d %H:%M", &tm1), ());
  TEST(strptime("1970-01-01 00:00", "%Y-%m-%d %H:%M", &tm2), ());
  TEST_EQUAL(timegm(&tm1), base::TimeGM(tm2), ());
  TEST_EQUAL(timegm(&tm1), base::TimeGM(1970, 1, 1, 0, 0, 0), ());

  TEST(strptime("2012-12-02 21:08:34", "%Y-%m-%d %H:%M:%S", &tm1), ());
  TEST(strptime("2012-12-02 21:08:34", "%Y-%m-%d %H:%M:%S", &tm2), ());
  TEST_EQUAL(timegm(&tm1), base::TimeGM(tm2), ());
  TEST_EQUAL(timegm(&tm1), base::TimeGM(2012, 12, 2, 21, 8, 34), ());
}
