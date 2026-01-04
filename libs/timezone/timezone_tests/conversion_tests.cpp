#include <gtest/gtest.h>

#include "timezone/serdes.hpp"

using namespace om::tz;

namespace
{
constexpr TimeZone kZeroTz{.generation_year_offset = 0, .base_offset = 64, .dst_delta = 0, .transitions = {}};

time_t CreateTime(int const year, int const month, int const day, int const hour, int const minute,
                       int const second)
{
  std::tm tm_time{};
  tm_time.tm_year = year - 1900;  // tm_year is years since 1900
  tm_time.tm_mon = month - 1;     // tm_mon is 0-based
  tm_time.tm_mday = day;
  tm_time.tm_hour = hour;
  tm_time.tm_min = minute;
  tm_time.tm_sec = second;
  tm_time.tm_isdst = -1;

  return timegm(&tm_time);
}

std::string TimeToString(time_t const t)
{
  std::tm const tm = *std::gmtime(&t);
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}
}  // namespace

#define EXPECT_EQ_TIME(expected, actual) EXPECT_EQ(TimeToString(expected), TimeToString(actual))

TEST(TimeZoneConvert, ShoudNotChangeTimeWhenEqualTimeZones)
{
  time_t const time = CreateTime(2025, 12, 13, 21, 18, 16);

  EXPECT_EQ_TIME(time, Convert(time, kZeroTz, kZeroTz));
}

TEST(TimeZoneConvert, ShoudAdd1Hour)
{
  TimeZone const dstTz{.generation_year_offset = 0, .base_offset = 68, .dst_delta = 0, .transitions = {}};

  {
    time_t const srcTime = CreateTime(2025, 12, 13, 21, 18, 16);
    time_t const dstTime = CreateTime(2025, 12, 13, 22, 18, 16);

    EXPECT_EQ_TIME(dstTime, Convert(srcTime, kZeroTz, dstTz));
    EXPECT_EQ_TIME(srcTime, Convert(dstTime, dstTz, kZeroTz));
  }
  {
    time_t const srcTime = CreateTime(2025, 12, 13, 23, 18, 16);
    time_t const dstTime = CreateTime(2025, 12, 14, 0, 18, 16);

    EXPECT_EQ_TIME(dstTime, Convert(srcTime, kZeroTz, dstTz));
    EXPECT_EQ_TIME(srcTime, Convert(dstTime, dstTz, kZeroTz));
  }
}

TEST(TimeZoneConvert, ShoudDecrease1Hour)
{
  TimeZone const srcTz{.generation_year_offset = 0, .base_offset = 68, .dst_delta = 0, .transitions = {}};

  {
    time_t const srcTime = CreateTime(2025, 12, 13, 22, 18, 16);
    time_t const dstTime = CreateTime(2025, 12, 13, 21, 18, 16);

    EXPECT_EQ_TIME(dstTime, Convert(srcTime, srcTz, kZeroTz));
    EXPECT_EQ_TIME(srcTime, Convert(dstTime, kZeroTz, srcTz));
  }
  {
    time_t const srcTime = CreateTime(2025, 12, 13, 0, 18, 16);
    time_t const dstTime = CreateTime(2025, 12, 12, 23, 18, 16);

    EXPECT_EQ_TIME(dstTime, Convert(srcTime, srcTz, kZeroTz));
    EXPECT_EQ_TIME(srcTime, Convert(dstTime, kZeroTz, srcTz));
  }
}

TEST(TimeZoneConvert, ShouldApplyDst)
{
  // Example: base offset = 0, DST +60 minutes
  TimeZone const dstTz{.generation_year_offset = 0,
                       .base_offset = 64,
                       .dst_delta = 60,
                       .transitions = {
                           {.day_delta = 67, .minute_of_day = 120},   // DST starts 2025, Mar 9, 2:00
                           {.day_delta = 238, .minute_of_day = 120},  // DST ends 2025, Nov 2, 2:00
                           {.day_delta = 127, .minute_of_day = 120},  // DST starts 2026, Mar 9, 2:00
                           {.day_delta = 238, .minute_of_day = 120},  // DST ends 2026, Nov 2, 2:00
                       }};

  for (int const year : {2025, 2026})
  {
    {
      // Before DST starts
      time_t const timeBeforeDst = CreateTime(year, 3, 9, 1, 30, 0);
      time_t const expected = CreateTime(year, 3, 9, 1, 30, 0);  // no offset yet
      EXPECT_EQ_TIME(expected, Convert(timeBeforeDst, kZeroTz, dstTz));
      EXPECT_EQ_TIME(timeBeforeDst, Convert(expected, dstTz, kZeroTz));
    }

    {
      // After DST starts
      time_t const timeAfterDst = CreateTime(year, 3, 9, 3, 30, 0);
      time_t const expected = CreateTime(year, 3, 9, 4, 30, 0);  // +1 hour DST
      EXPECT_EQ_TIME(expected, Convert(timeAfterDst, kZeroTz, dstTz));
      EXPECT_EQ_TIME(timeAfterDst, Convert(expected, dstTz, kZeroTz));
    }

    {
      // Before DST ends
      time_t const timeBeforeEnd = CreateTime(year, 11, 2, 1, 30, 0);
      time_t const expected = CreateTime(year, 11, 2, 2, 30, 0);  // still DST
      EXPECT_EQ_TIME(expected, Convert(timeBeforeEnd, kZeroTz, dstTz));
      // EXPECT_EQ_TIME(timeBeforeEnd, Convert(expected, dstTz, kZeroTz));
    }

    {
      // After DST ends
      time_t const timeAfterEnd = CreateTime(year, 11, 2, 2, 30, 0);
      time_t const expected = CreateTime(year, 11, 2, 2, 30, 0);  // back to a standard
      EXPECT_EQ_TIME(expected, Convert(timeAfterEnd, kZeroTz, dstTz));
      EXPECT_EQ_TIME(timeAfterEnd, Convert(expected, dstTz, kZeroTz));
    }
  }
}

TEST(TimeZoneConvert, CrossTimeZoneWithOffsets)
{
  // Source timezone: -2:30 (minutes = -150), no DST
  TimeZone const srcTz{.generation_year_offset = 0,
                       .base_offset = 54,  // -2:30 hours
                       .dst_delta = 0,
                       .transitions = {}};

  // Destination timezone: +8:00, DST +60 minutes
  TimeZone const dstTz{.generation_year_offset = 0,
                       .base_offset = 96,  // +8 hours
                       .dst_delta = 60,     // +1 hour DST
                       .transitions = {
                           {.day_delta = 67, .minute_of_day = 451},   // DST starts Mar 9, 7:31
                           {.day_delta = 238, .minute_of_day = 218},  // DST ends Nov 2, 3:38
                       }};

  // Test before DST starts
  {
    time_t const localSrc = CreateTime(2025, 3, 8, 21, 0, 0);     // local in srcTz
    time_t const utcTime = CreateTime(2025, 3, 8, 23, 30, 0);     // +2:30 = UTC 23:30
    time_t const expectedDst = CreateTime(2025, 3, 9, 7, 30, 0);  // UTC = 23:30, +8:00 = 7:30 (next day)
    EXPECT_EQ_TIME(utcTime, Convert(localSrc, srcTz, kZeroTz));
    EXPECT_EQ_TIME(expectedDst, Convert(utcTime, kZeroTz, dstTz));
    EXPECT_EQ_TIME(expectedDst, Convert(localSrc, srcTz, dstTz));
    EXPECT_EQ_TIME(localSrc, Convert(expectedDst, dstTz, srcTz));
  }

  // Test after DST starts
  {
    time_t const localSrc = CreateTime(2025, 3, 9, 5, 1, 1);       // local in srcTz
    time_t const utcTime = CreateTime(2025, 3, 9, 7, 31, 1);       // +2:30 = UTC 7:31:01
    time_t const expectedDst = CreateTime(2025, 3, 9, 16, 31, 1);  // UTC = 7:31:01, +8:00 + 1:00 DST = 16:31:01
    EXPECT_EQ_TIME(utcTime, Convert(localSrc, srcTz, kZeroTz));
    EXPECT_EQ_TIME(expectedDst, Convert(utcTime, kZeroTz, dstTz));
    EXPECT_EQ_TIME(expectedDst, Convert(localSrc, srcTz, dstTz));
    EXPECT_EQ_TIME(localSrc, Convert(expectedDst, dstTz, srcTz));
  }

  // Test before DST ends
  {
    time_t const localSrc = CreateTime(2025, 11, 1, 16, 7, 59);  // local in srcTz
    time_t const utcTime = CreateTime(2025, 11, 1, 18, 37, 59);  // +2:30 = UTC 18:37:59
    time_t const expectedDst =
        CreateTime(2025, 11, 2, 3, 37, 59);  // UTC = 18:37:59, +8:00 + 1:00 DST = 3:37:59 (next day)
    EXPECT_EQ_TIME(utcTime, Convert(localSrc, srcTz, kZeroTz));
    EXPECT_EQ_TIME(expectedDst, Convert(utcTime, kZeroTz, dstTz));
    EXPECT_EQ_TIME(expectedDst, Convert(localSrc, srcTz, dstTz));
    EXPECT_EQ_TIME(localSrc, Convert(expectedDst, dstTz, srcTz));
  }

  // Test after DST ends
  {
    time_t const localSrc = CreateTime(2025, 11, 2, 3, 0, 0);       // local in srcTz
    time_t const utcTime = CreateTime(2025, 11, 2, 5, 30, 0);       // +2:30 = UTC 5:30
    time_t const expectedDst = CreateTime(2025, 11, 2, 13, 30, 0);  // UTC = 5:30, +8:00 = 13:30
    EXPECT_EQ_TIME(utcTime, Convert(localSrc, srcTz, kZeroTz));
    EXPECT_EQ_TIME(expectedDst, Convert(utcTime, kZeroTz, dstTz));
    EXPECT_EQ_TIME(expectedDst, Convert(localSrc, srcTz, dstTz));
    EXPECT_EQ_TIME(localSrc, Convert(expectedDst, dstTz, srcTz));
  }
}
