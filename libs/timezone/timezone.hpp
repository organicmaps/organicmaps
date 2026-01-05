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
  static constexpr size_t kDayDeltaBitSize = 16;
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

  constexpr auto operator<=>(TimeZone const & rhs) const = default;
};

struct TimeZoneDb
{
  std::string tzdb_version;
  uint8_t tzdb_format_version;
  std::uint16_t tzdb_generation_year_offset;
  std::unordered_map<std::string, TimeZone> timezones;
};

time_t Convert(time_t time, TimeZone const & srcTimeZone, TimeZone const & dstTimeZone);

/// @warning Do not call in runtime. Only for generator and testing.
TimeZoneDb const & GetTimeZoneDb();
}  // namespace om::tz
