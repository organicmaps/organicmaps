#pragma once

#include "std/cstdint.hpp"
#include "std/string.hpp"
#include "std/ctime.hpp"
#include "std/chrono.hpp"


namespace my
{

/// Cross platform timer
class Timer
{
  double m_startTime;

public:
  explicit Timer(bool start = true);

  /// @return current UTC time in seconds, elapsed from 1970.
  static double LocalTime();
  /// @return Elapsed time in seconds from start (@see Reset).
  double ElapsedSeconds() const;
  void Reset();
};

string FormatCurrentTime();

/// Generates timestamp for a specified day.
/// \param year  The number of years since 1900.
/// \param month The number of month since January, in the range 0 to 11.
/// \param day   The day of the month, in the range 1 to 31.
/// \return Timestamp.
uint32_t GenerateTimestamp(int year, int month, int day);

uint32_t TodayAsYYMMDD();

/// Always creates strings in UTC time: 1997-07-16T07:30:15Z
/// Returns empty string on error
string TimestampToString(time_t time);

time_t const INVALID_TIME_STAMP = -1;

/// Accepts strings in UTC format: 1997-07-16T07:30:15Z
/// And with custom time offset:   1997-07-16T10:30:15+03:00
/// @return INVALID_TIME_STAMP if string is invalid
time_t StringToTimestamp(string const & s);


/// High resolution timer to use in comparison tests.
class HighResTimer
{
  typedef high_resolution_clock::time_point PointT;
  PointT m_start;

public:
  explicit HighResTimer(bool start = true);

  void Reset();
  uint64_t ElapsedNano() const;
};

}
