#pragma once

#include <expected>

#include "timezone/timezone.hpp"

namespace om::tz
{
enum class SerializationError
{
  IncorrectHeader,
  IncorrectTransitionsFormat,
  IncorrectGenerationYearOffsetFormat,
  IncorrectBaseOffsetFormat,
  IncorrectDstDeltaFormat,
  IncorrectTransitionsLengthFormat,
  IncorrectTransitionsAmount,
  IncorrectDayDeltaFormat,
  IncorrectMinuteOfDayFormat
};

std::expected<std::string, SerializationError> Serialize(TimeZone const & timeZone);
std::expected<TimeZone, SerializationError> Deserialize(std::string_view const & data);

std::string DebugPrint(SerializationError error);
}  // namespace om::tz
