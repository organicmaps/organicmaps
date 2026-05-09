#pragma once

#include <expected>
#include <string_view>

#include "timezone/timezone.hpp"

namespace om::tz
{
enum class SerializationError
{
  IncorrectHeader,
  IncorrectTransitionsFormat,
  UnsupportedTimeZoneFormat,
  IncorrectGenerationYearOffsetFormat,
  IncorrectBaseOffsetFormat,
  IncorrectDstDeltaFormat,
  IncorrectTransitionsLengthFormat,
  IncorrectTransitionsAmount,
  IncorrectDayDeltaFormat,
  IncorrectMinuteOfDayFormat
};

std::expected<std::string, SerializationError> Serialize(TimeZone const & timeZone);
std::expected<TimeZone, SerializationError> Deserialize(std::string_view data);

std::string_view DebugPrint(SerializationError error);
}  // namespace om::tz
