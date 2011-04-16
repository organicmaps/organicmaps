#pragma once
#include "base.hpp"
#include <boost/date_time/posix_time/posix_time.hpp>

namespace my
{
  /// Cross platform timer
  class Timer
  {
  public:
    Timer() { Reset(); }
    double ElapsedSeconds() const
    {
      return (boost::posix_time::microsec_clock::local_time() - m_StartTime).total_milliseconds()
          / 1000.0;
    }

    void Reset() { m_StartTime = boost::posix_time::microsec_clock::local_time(); }
  private:
    boost::posix_time::ptime m_StartTime;
  };

}
