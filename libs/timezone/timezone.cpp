#include "timezone.hpp"

namespace om::tz
{
namespace
{
constexpr std::time_t kSecondsPerDay = 86400;
constexpr std::time_t kSecondsPerMinute = 60;

constexpr int64_t DaysUntilYear(int const year)
{
  int const y = year - 1;  // last year included
  int64_t const days = (y - 1970 + 1) * 365;

  // count leap days
  int64_t const leaps = (y / 4 - 1969 / 4) - (y / 100 - 1969 / 100) + (y / 400 - 1969 / 400);

  return days + leaps;
}

std::time_t GenerationYearStart(int const generation_year_offset)
{
  int const generation_year = TimeZone::kGenerationYearStart + generation_year_offset;
  return DaysUntilYear(generation_year) * kSecondsPerDay;
}

// Compute offset at UTC timestamp deterministically
int32_t TzOffsetAtUtc(TimeZone const & timeZone, std::time_t const utcTime)
{
  int32_t offset = timeZone.base_offset;

  std::time_t const startOfYear = GenerationYearStart(timeZone.generation_year_offset);
  int64_t dayOffset = 0;

  for (auto const & [dayDelta, minute_of_day, is_dst] : timeZone.transitions)
  {
    dayOffset += dayDelta;

    std::time_t const utcTransition = startOfYear + dayOffset * kSecondsPerDay + minute_of_day * kSecondsPerMinute;

    if (utcTime >= utcTransition)
      offset = timeZone.base_offset + (is_dst ? timeZone.dst_delta : 0);
    else
      break;
  }

  return offset;
}

// Compute UTC from local time handling ambiguous DST times
std::time_t LocalToUtc(TimeZone const & tz, std::time_t localTime)
{
  // Initial guess using base offset
  int32_t offset = tz.base_offset;
  std::time_t utc = localTime - offset * kSecondsPerMinute;

  for (int i = 0; i < 2; ++i)  // handle rare ambiguous DST hour
  {
    int32_t candidate = TzOffsetAtUtc(tz, utc);
    std::time_t newUtc = localTime - candidate * kSecondsPerMinute;

    if (newUtc == utc)
      break;  // converged
    utc = newUtc;
    offset = candidate;
  }

  // Ambiguous fall-back: UTC corresponds to two possible offsets
  // Pick DST-active if it exists at that UTC (you can also choose earliest/standard)
  int32_t firstOffset = TzOffsetAtUtc(tz, utc - kSecondsPerMinute);   // just before UTC
  int32_t secondOffset = TzOffsetAtUtc(tz, utc + kSecondsPerMinute);  // just after UTC

  if (firstOffset != secondOffset)
  {
    // DST ends: pick the DST-active side if localTime matches ambiguous hour
    int32_t dstOffset = (firstOffset > secondOffset) ? firstOffset : secondOffset;
    utc = localTime - dstOffset * kSecondsPerMinute;
  }

  return utc;
}

}  // namespace

std::time_t Convert(std::time_t const localTime, TimeZone const & srcTimeZone, TimeZone const & dstTimeZone)
{
  // Step 1: Convert local → UTC handling DST ambiguity
  std::time_t utcTime = LocalToUtc(srcTimeZone, localTime);

  // Step 2: Convert UTC → dst local
  int32_t dstOffset = TzOffsetAtUtc(dstTimeZone, utcTime);
  return utcTime + dstOffset * kSecondsPerMinute;
}

}  // namespace om::tz
