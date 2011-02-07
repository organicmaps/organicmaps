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
    Timer() {Reset();}
    double ElapsedSeconds() const
    {
      return (clock() - m_StartTime) / static_cast<double>(CLOCKS_PER_SEC);
    }

    void Reset() { m_StartTime = clock(); }
  private:
    clock_t m_StartTime;
  };

}
