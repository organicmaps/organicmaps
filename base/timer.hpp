#pragma once
#include "base.hpp"
#include "../std/ctime.hpp"

namespace my
{
  // TODO: Check std::clock() on iPhone and iPhone Simulator.

  // Cross platform timer.
  class Timer
  {
  public:
    Timer() : m_StartTime(clock()) {}
    double ElapsedSeconds() const
    {
      return (clock() - m_StartTime) / static_cast<double>(CLOCKS_PER_SEC);
    }
  private:
    clock_t m_StartTime;
  };

}
