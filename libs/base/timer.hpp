#pragma once

#include <chrono>
#include <cstdint>
#include <ctime>
#include <string>

namespace base
{
namespace impl
{
template <class ClockT>
class StdTimer
{
  typename ClockT::time_point m_startTime;

public:
  explicit StdTimer(bool start = true)
  {
    if (start)
      Reset();
  }

  using DurationT = typename ClockT::duration;
  /// @return Elapsed time from start (@see Reset).
  DurationT TimeElapsed() const { return ClockT::now() - m_startTime; }

  template <typename Duration>
  Duration TimeElapsedAs() const
  {
    return std::chrono::duration_cast<Duration>(TimeElapsed());
  }

  double ElapsedSeconds() const { return TimeElapsedAs<std::chrono::duration<double>>().count(); }
  uint64_t ElapsedMilliseconds() const { return TimeElapsedAs<std::chrono::milliseconds>().count(); }
  uint64_t ElapsedNanoseconds() const { return TimeElapsedAs<std::chrono::nanoseconds>().count(); }

  void Reset() { m_startTime = ClockT::now(); }
};
}  // namespace impl

/// Cross platform timer
class Timer : public impl::StdTimer<std::chrono::steady_clock>
{
  using BaseT = impl::StdTimer<std::chrono::steady_clock>;

public:
  using BaseT::BaseT;

  /// @return current UTC time in seconds, elapsed from 1970.
  static double LocalTime();
};

std::string FormatCurrentTime();

/// Generates timestamp for a specified day.
/// \param year     The number of years since 1900.
/// \param month    The number of month since January, in the range 0 to 11.
/// \param day      The day of the month, in the range 1 to 31.
/// \return Timestamp.
uint32_t GenerateYYMMDD(int year, int month, int day);
uint32_t GenerateYYMMDD(uint64_t secondsSinceEpoch);
uint64_t YYMMDDToSecondsSinceEpoch(uint32_t yymmdd);

uint64_t SecondsSinceEpoch();

/// Always creates strings in UTC time: 1997-07-16T07:30:15Z
/// Returns empty string on error
std::string TimestampToString(time_t time);

std::string SecondsSinceEpochToString(uint64_t secondsSinceEpoch);

time_t const INVALID_TIME_STAMP = -1;

/// Accepts strings in UTC format: 1997-07-16T07:30:15Z
/// And with custom time offset:   1997-07-16T10:30:15+03:00
/// @return INVALID_TIME_STAMP if string is invalid
time_t StringToTimestamp(std::string const & s);

/// High resolution timer to use in comparison tests.
using HighResTimer = impl::StdTimer<std::chrono::high_resolution_clock>;

class ScopedTimerWithLog
{
public:
  enum class Measure
  {
    MilliSeconds,
    Seconds,
  };

  explicit ScopedTimerWithLog(std::string const & timerName, Measure measure = Measure::MilliSeconds);
  ~ScopedTimerWithLog();

private:
  std::string m_name;
  Measure m_measure;
  HighResTimer m_timer;
};

time_t SecondsSinceEpochToTimeT(uint64_t secondsSinceEpoch);
uint64_t TimeTToSecondsSinceEpoch(time_t time);
}  // namespace base
