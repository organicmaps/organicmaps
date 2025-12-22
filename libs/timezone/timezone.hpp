#pragma once

#include <ctime>
#include <unordered_map>
#include <vector>

namespace om::tz
{
struct Transition
{
  static constexpr int kDayDeltaBitSize = 16;
  static constexpr int kMinuteOfDayBitSize = 11;
  static constexpr int kIsDstBitSize = 1;

  // uint16_t day_delta : kDayDeltaBitSize;
  // uint16_t minute_of_day : kMinuteOfDayBitSize;
  // uint16_t is_dst : kIsDstBitSize;
  uint16_t day_delta;
  uint16_t minute_of_day;
  uint16_t is_dst;

  constexpr auto operator<=>(Transition const & rhs) const = default;
};

struct TimeZone
{
  static constexpr uint16_t kGenerationYearStart = 2025;

  static constexpr int kGenerationYearBitSize = 7;
  static constexpr int kBaseOffsetBitSize = 12;
  static constexpr int kDstDeltaBitSize = 8;
  static constexpr int kTransitionsLengthBitSize = 4;

  /// @brief The generation year of tzdb since 2025
  /// @details Why 2025? Because we implemented it that year. Smaller number -> fewer bits to store
  /// @note Consider increasing the number of bits after the year 2150 :)
  // uint16_t generation_year_offset : kGenerationYearBitSize;
  // int16_t base_offset : kBaseOffsetBitSize;
  // int16_t dst_delta : kDstDeltaBitSize;
  uint16_t generation_year_offset;
  int16_t base_offset;
  int16_t dst_delta;
  std::vector<Transition> transitions;

  constexpr auto operator<=>(TimeZone const & rhs) const = default;
};

struct TimeZoneDb
{
  std::string tzdb_version;
  std::uint16_t tzdb_generation_year_offset;
  std::unordered_map<std::string, TimeZone> timezones;
};

std::time_t Convert(std::time_t time, TimeZone const & srcTimeZone, TimeZone const & dstTimeZone);
}  // namespace om::tz
