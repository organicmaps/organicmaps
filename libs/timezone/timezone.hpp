#pragma once

#include <climits>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace om::tz
{
enum class TimeZoneFormatVersion : uint8_t
{
  V1 = 0,

  Count
};

struct Transition
{
  static constexpr size_t kDayDeltaBitSize = 9;
  static constexpr size_t kMinuteOfDayBitSize = 11;
  static constexpr size_t kTotalSizeInBits = kDayDeltaBitSize + kMinuteOfDayBitSize;
  static constexpr size_t kTotalSizeInBytes = (kTotalSizeInBits + CHAR_BIT - 1) / CHAR_BIT;

  uint16_t day_delta;
  uint16_t minute_of_day;

  constexpr auto operator<=>(Transition const & rhs) const = default;
};

struct TimeZone
{
  static constexpr uint16_t kGenerationYearStart = 2026;

  static constexpr size_t kFormatVersionBitSize = 3;
  static constexpr size_t kGenerationYearBitSize = 6;
  static constexpr size_t kBaseOffsetBitSize = 7;
  static constexpr size_t kDstDeltaBitSize = 8;
  static constexpr size_t kTransitionsLengthBitSize = 4;
  static constexpr size_t kTotalSizeInBits = kFormatVersionBitSize + kGenerationYearBitSize + kBaseOffsetBitSize +
                                             kDstDeltaBitSize + kTransitionsLengthBitSize;
  static constexpr size_t kTotalSizeInBytes = (kTotalSizeInBits + CHAR_BIT - 1) / CHAR_BIT;

  TimeZoneFormatVersion format_version = TimeZoneFormatVersion::V1;
  uint16_t generation_year_offset;
  uint8_t base_offset;
  uint8_t dst_delta;
  std::vector<Transition> transitions;

  int32_t GetBaseOffset() const { return (static_cast<int32_t>(base_offset) - 64) * 15; }

  constexpr auto operator<=>(TimeZone const & rhs) const = default;
};

struct TimeZoneDb
{
  std::string tzdb_version;
  uint8_t tzdb_format_version;
  std::uint16_t tzdb_generation_year_offset;
  std::unordered_map<std::string, TimeZone> timezones;
};

/**
 * time_t stores the value in UTC-0 format by default, E.g.,
 * @code
 * time_t currentTime = std::time(nullptr);
 * @endcode
 * currentTime will be the number of seconds since "00:00, Jan 1 1970 UTC-0"
 *
 * We use time_t to store our own time format that respects time zones.  E.g.,
 * @code
 * time_t currentTime = std::time(nullptr); // 12:00, Mar 12 2026 UTC-0
 * TimeZone localTimeZone = ...; // UTC+3
 * ZonedTime currentLocalTime = Convert(currentTime, localTimeZone); // 15:00, Mar 12 2026 UTC+3
 *
 * currentTime + 3h == currentLocalTime
 * @endcode
 */
using ZonedTime = time_t;

/**
 * Converts time between time zones.
 *
 * @param time Time in the source time zone format. Make sure that you use ZonedTime not default time_t.
 * @param srcTimeZone Source time zone.
 * @param dstTimeZone Destination time zone.
 * @return Time in the destination time zone format.
 */
ZonedTime Convert(ZonedTime time, TimeZone const & srcTimeZone, TimeZone const & dstTimeZone);

/**
 * Converts the time from UTC-0 to the specified time zone.
 * This function is helpful when you need to convert localtime to the specific zone time.
 * E.g., when you use time(nullptr)
 *
 * @param time Time in UTC-0.
 * @param timeZone Time zone.
 * @return Time in the ZonedTime format for the specified time zone.
 */
ZonedTime Convert(time_t time, TimeZone const & timeZone);

/// @warning Do not call in runtime. Only for generator and testing.
TimeZoneDb const & GetTimeZoneDb();
}  // namespace om::tz
