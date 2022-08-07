#pragma once

#include <cstdint>
#include <ctime>

namespace base
{

// Returns true if year is leap (has 29 days in Feb).
bool IsLeapYear(int year);

// Returns number of days for specified month and year.
int DaysOfMonth(int year, int month);

// Android has timegm from API 21. MSVC does not have it.
time_t TimeGM(std::tm & tm);

// Forms timestamp (number of seconds since 1.1.1970) from year/day/month, hour:min:sec
// year - since 0, for example 2015
// month - 1-jan...12-dec
// day - 1...31
// hour - 0...23
// min - 0...59
// sec - 0...59
time_t TimeGM(int year, int month, int day, int hour, int min, int sec);
} // base
