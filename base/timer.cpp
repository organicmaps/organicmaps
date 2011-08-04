#include "timer.hpp"

#include "../std/target_os.hpp"
#include "../std/systime.hpp"
#include "../std/ctime.hpp"
#include "../std/stdint.hpp"

namespace my
{

Timer::Timer()
{
  Reset();
}

double Timer::LocalTime()
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

string FormatCurrentTime()
{
    time_t t = time(NULL);
    string s = string(ctime(&t));

    for (size_t i = 0; i < s.size(); ++i)
      if (s[i] == ' ') s[i] = '_';

    s.erase(s.size() - 1, 1);
    return s;
}

uint32_t TodayAsYYMMDD()
{
  time_t rawTime = time(NULL);
  tm * pTm = gmtime(&rawTime);
  return (pTm->tm_year - 100) * 10000 + (pTm->tm_mon + 1) * 100 + pTm->tm_mday;
}

}
