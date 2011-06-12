#include "timer.hpp"

#include "../std/target_os.hpp"
#include "../std/time.hpp"

namespace my
{

Timer::Timer()
{
  Reset();
}

double Timer::LocalTime() const
{
#ifdef OMIM_OS_WINDOWS
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  uint64_t val = ft.dwHighDateTime;
  val <<= 32;
  val += ft.dwLowDateTime;
  return val / 10000000.0;

#else
  timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec + tv.tv_usec / 1000000.0;
#endif
}

double Timer::ElapsedSeconds() const
{
  return LocalTime() - m_startTime;
}

void Timer::Reset()
{
  m_startTime = LocalTime();
}

}
