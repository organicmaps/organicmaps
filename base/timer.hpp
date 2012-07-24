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

  static double LocalTime();
  double ElapsedSeconds() const;
  void Reset();
};

string FormatCurrentTime();
uint32_t TodayAsYYMMDD();

}
