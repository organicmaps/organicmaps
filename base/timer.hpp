#pragma once

#include "../std/string.hpp"

namespace my
{

/// Cross platform timer
class Timer
{
  double m_startTime;

  static double LocalTime();

public:
  Timer();

  double ElapsedSeconds() const;
  void Reset();
};

string FormatCurrentTime();
uint32_t TodayAsYYMMDD();

}
