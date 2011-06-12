#pragma once

namespace my
{

/// Cross platform timer
class Timer
{
  double m_startTime;

  double LocalTime() const;

public:
  Timer();
  double ElapsedSeconds() const;
  void Reset();
};

}
