#include "timezone.hpp"

#include "platform/platform.hpp"

#include <glaze/json.hpp>

namespace om::tz
{
namespace
{
constexpr time_t kSecondsPerDay = 86400;
constexpr time_t kSecondsPerMinute = 60;

constexpr int64_t DaysUntilYear(int const year)
{
  int const y = year - 1;  // last year included
  int64_t const days = (y - 1970 + 1) * 365;

  // count leap days
  int64_t const leaps = (y / 4 - 1969 / 4) - (y / 100 - 1969 / 100) + (y / 400 - 1969 / 400);

  return days + leaps;
}

time_t GenerationYearStart(int const generation_year_offset)
{
  int const generation_year = TimeZone::kGenerationYearStart + generation_year_offset;
  return DaysUntilYear(generation_year) * kSecondsPerDay;
}

int32_t DecodeBaseOffset(uint8_t const baseOffset)
{
  return (static_cast<int32_t>(baseOffset) - 64) * 15;
}

// Compute offset at UTC timestamp deterministically
int32_t TzOffsetAtUtc(TimeZone const & timeZone, time_t const utcTime)
{
  int32_t const baseOffset = DecodeBaseOffset(timeZone.base_offset);
  int32_t offset = baseOffset;

  time_t const startOfYear = GenerationYearStart(timeZone.generation_year_offset);
  int64_t dayOffset = 0;

  for (auto const & [dayDelta, minute_of_day, is_dst] : timeZone.transitions)
  {
    dayOffset += dayDelta;

    time_t const utcTransition = startOfYear + dayOffset * kSecondsPerDay + minute_of_day * kSecondsPerMinute;

    if (utcTime >= utcTransition)
      offset = baseOffset + (is_dst ? timeZone.dst_delta : 0);
    else
      break;
  }

  return offset;
}

// Compute UTC from local time handling ambiguous DST times
time_t LocalToUtc(TimeZone const & tz, time_t const localTime)
{
  // Initial guess using base offset
  int32_t offset = DecodeBaseOffset(tz.base_offset);
  time_t utc = localTime - offset * kSecondsPerMinute;

  for (int i = 0; i < 2; ++i)  // handle rare ambiguous DST hour
  {
    int32_t const candidate = TzOffsetAtUtc(tz, utc);
    time_t const newUtc = localTime - candidate * kSecondsPerMinute;

    if (newUtc == utc)
      break;  // converged
    utc = newUtc;
    offset = candidate;
  }

  // Ambiguous fall-back: UTC corresponds to two possible offsets
  // Pick DST-active if it exists at that UTC (you can also choose earliest/standard)
  int32_t const firstOffset = TzOffsetAtUtc(tz, utc - kSecondsPerMinute);   // just before UTC
  int32_t const secondOffset = TzOffsetAtUtc(tz, utc + kSecondsPerMinute);  // just after UTC

  if (firstOffset != secondOffset)
  {
    // DST ends: pick the DST-active side if localTime matches ambiguous hour
    int32_t const dstOffset = (firstOffset > secondOffset) ? firstOffset : secondOffset;
    utc = localTime - dstOffset * kSecondsPerMinute;
  }

  return utc;
}

}  // namespace

time_t Convert(time_t const time, TimeZone const & srcTimeZone, TimeZone const & dstTimeZone)
{
  // Step 1: Convert local → UTC handling DST ambiguity
  time_t const utcTime = LocalToUtc(srcTimeZone, time);

  // Step 2: Convert UTC → dst local
  int32_t const dstOffset = TzOffsetAtUtc(dstTimeZone, utcTime);
  return utcTime + dstOffset * kSecondsPerMinute;
}

TimeZoneDb const & GetTimeZoneDb()
{
  static std::optional<TimeZoneDb> tzdb;
  if (tzdb)
    return *tzdb;

  tzdb.emplace();
  std::string buffer;
  GetPlatform().GetReader(TIMEZONE_INFO_FILE)->ReadAsString(buffer);
  if (auto const ec = glz::read_json(*tzdb, buffer); ec.ec != glz::error_code::none)
    LOG(LERROR, ("Failed to load timezone database. ec:", ec.ec, "custom_error_message:", ec.custom_error_message));

  for (auto & tz : tzdb->timezones | std::views::values)
    tz.generation_year_offset = tzdb->tzdb_generation_year_offset;

  return *tzdb;
}
}  // namespace om::tz
