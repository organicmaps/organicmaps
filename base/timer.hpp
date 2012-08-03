#pragma once

#include "../std/stdint.hpp"
#include "../std/string.hpp"
#include "../std/stdint.hpp"

namespace my
{

/// Cross platform timer
class Timer
{
  double m_startTime;

public:
  Timer();

  /// @return current UTC time in seconds, elapsed from 1970.
  static double LocalTime();
  /// @return Elapsed time in seconds from start (@see Reset).
  double ElapsedSeconds() const;
  void Reset();
};

string FormatCurrentTime();
uint32_t TodayAsYYMMDD();

}
