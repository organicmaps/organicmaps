#pragma once

#include "timezone/timezone.hpp"

namespace om::tz
{
enum class SerializationError
{
  OK,
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

SerializationError Serialize(TimeZone const & timeZone, std::string & buf);
SerializationError Deserialize(std::string_view const & data, TimeZone & res);

std::string DebugPrint(SerializationError error);
}  // namespace om::tz
