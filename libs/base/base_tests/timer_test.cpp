#include "testing/testing.hpp"

#include "base/logging.hpp"
#include "base/timer.hpp"

UNIT_TEST(Timer_Seconds)
{
  base::Timer timer;

  double t1 = timer.ElapsedSeconds();
  double s = 0.0;
  for (int i = 0; i < 10000000; ++i)
    s += i * 0.01;
  double t2 = timer.ElapsedSeconds();

  TEST_NOT_EQUAL(s, 0.0, ("Fictive, to prevent loop optimization"));
  TEST_NOT_EQUAL(t1, t2, ("Timer values should not be equal"));
}

UNIT_TEST(Timer_CurrentStringTime)
{
  LOG(LINFO, (base::FormatCurrentTime()));
}

UNIT_TEST(Timer_TimestampConversion)
{
  using namespace base;

  TEST_EQUAL(TimestampToString(0), "1970-01-01T00:00:00Z", ());
  TEST_EQUAL(TimestampToString(1354482514), "2012-12-02T21:08:34Z", ());

  TEST_EQUAL(StringToTimestamp("1970-01-01T00:00:00Z"), 0, ());
  TEST_EQUAL(StringToTimestamp("1970-01-01T00:00:00.1Z"), 0, ());
  TEST_EQUAL(StringToTimestamp("1970-01-01T00:00:00.12Z"), 0, ());
  TEST_EQUAL(StringToTimestamp("1970-01-01T00:00:00.123Z"), 0, ());
  TEST_EQUAL(StringToTimestamp("1970-01-01T00:00:00.000009Z"), 0, ());
  TEST_EQUAL(StringToTimestamp("2012-12-02T21:08:34"), 1354482514, ());
  TEST_EQUAL(StringToTimestamp("2012-12-02T21:08:34Z"), 1354482514, ());
  TEST_EQUAL(StringToTimestamp("2012-12-03T00:38:34+03:30"), 1354482514, ());
  TEST_EQUAL(StringToTimestamp("2012-12-02T11:08:34-10:00"), 1354482514, ());
  TEST_EQUAL(StringToTimestamp("2014-09-30T23:59:59+23:59"), 1412035259, ());

  time_t const now = time(0);
  TEST_EQUAL(now, StringToTimestamp(TimestampToString(now)), ());

  TEST_EQUAL(INVALID_TIME_STAMP, StringToTimestamp("asd23423adsfbhj657"), ());

  TEST_EQUAL(INVALID_TIME_STAMP, StringToTimestamp("2012-aa-02T21:08:34Z"), ());
  TEST_EQUAL(INVALID_TIME_STAMP, StringToTimestamp("2012-12-0ZT21:08:34Z"), ());
  TEST_EQUAL(INVALID_TIME_STAMP, StringToTimestamp("2012:12:02T21-08-34Z"), ());

  TEST_EQUAL(INVALID_TIME_STAMP, StringToTimestamp("2012-"), ());
  TEST_EQUAL(INVALID_TIME_STAMP, StringToTimestamp("2012-12-02"), ());
  TEST_EQUAL(INVALID_TIME_STAMP, StringToTimestamp("1970-01-01T"), ());
  TEST_EQUAL(INVALID_TIME_STAMP, StringToTimestamp("1970-01-01T21:"), ());

  TEST_EQUAL(INVALID_TIME_STAMP, StringToTimestamp("2000-00-02T11:08:34-10:00"), ());
  TEST_EQUAL(INVALID_TIME_STAMP, StringToTimestamp("2000-13-02T11:08:34-10:00"), ());
  TEST_EQUAL(INVALID_TIME_STAMP, StringToTimestamp("2000-01-00T11:08:34-10:00"), ());
  TEST_EQUAL(INVALID_TIME_STAMP, StringToTimestamp("2000-01-32T11:08:34-10:00"), ());
  TEST_EQUAL(INVALID_TIME_STAMP, StringToTimestamp("2100-01--1T11:08:34-10:00"), ());
  TEST_EQUAL(INVALID_TIME_STAMP, StringToTimestamp("2100--1-02T11:08:34-10:00"), ());
  TEST_EQUAL(INVALID_TIME_STAMP, StringToTimestamp("2012-12-02T11:08:34-25:88"), ());
}

UNIT_TEST(Timer_GenerateYYMMDD)
{
  TEST_EQUAL(base::GenerateYYMMDD(116, 0, 26), 160126, ());
}

UNIT_TEST(Timer_TimeTConversion)
{
  auto const now = ::time(nullptr);
  TEST_EQUAL(base::SecondsSinceEpochToTimeT(base::TimeTToSecondsSinceEpoch(now)), now, ());
}
