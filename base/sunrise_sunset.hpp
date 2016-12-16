#pragma once

#include <ctime>
#include <string>

enum class DayTimeType
{
  Day,
  Night,
  PolarDay,
  PolarNight
};

std::string DebugPrint(DayTimeType type);

/// Helpers which calculates 'is day time' without a time calculation error.
/// @param timeUtc - utc time
/// @param latitude - latutude, -90...+90 degrees
/// @param longitude - longitude, -180...+180 degrees
/// @returns day time type for a specified date for a specified location
/// @note throws RootException if gmtime returns nullptr
DayTimeType GetDayTime(time_t timeUtc, double latitude, double longitude);
