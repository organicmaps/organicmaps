#include "timer.hpp"
#include "assert.hpp"
#include "macros.hpp"

#include "../std/target_os.hpp"
#include "../std/systime.hpp"
#include "../std/cstdio.hpp"
#include "../std/memcpy.hpp"

namespace my
{

Timer::Timer()
{
  Reset();
}

double Timer::LocalTime()
{
#ifdef OMIM_OS_WINDOWS_NATIVE
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
  string s(ctime(&t));

  replace(s.begin(), s.end(), ' ', '_');

  ASSERT_GREATER(s.size(), 1, ());
  s.resize(s.size() - 1);
  return s;
}

uint32_t TodayAsYYMMDD()
{
  time_t rawTime = time(NULL);
  tm * pTm = gmtime(&rawTime);
  return (pTm->tm_year - 100) * 10000 + (pTm->tm_mon + 1) * 100 + pTm->tm_mday;
}

namespace
{
  time_t my_timegm(tm * tm)
  {
#ifdef OMIM_OS_ANDROID
    char * tz = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    time_t ret = mktime(tm);
    if (tz)
      setenv("TZ", tz, 1);
    else
      unsetenv("TZ");
    tzset();

    return ret;
#elif defined(OMIM_OS_WINDOWS)
    return mktime(tm);
#else
    return timegm(tm);
#endif
  }
}

string TimestampToString(time_t time)
{
  tm * t = gmtime(&time);
  char buf[21] = { 0 };
#ifdef OMIM_OS_WINDOWS
  sprintf_s(buf, ARRAY_SIZE(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ", t->tm_year + 1900,
            t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
#else
  ::snprintf(buf, ARRAY_SIZE(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ", t->tm_year + 1900,
             t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
#endif

  return buf;
}

time_t StringToTimestamp(string const & s)
{
  tm t;
  memset(&t, 0, sizeof(t));
  // Return current time in the case of failure
  time_t res = INVALID_TIME_STAMP;
  // Parse UTC format
  if (s.size() == 20)
  {
    if (6 == sscanf(s.c_str(), "%4d-%2d-%2dT%2d:%2d:%2dZ", &t.tm_year,
                    &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec))
    {
      t.tm_year -= 1900;
      t.tm_mon -= 1;
      res = my_timegm(&t);
    }
  }
  else if (s.size() == 25)
  {
    // Parse custom time zone offset format
    char sign;
    int tzHours, tzMinutes;
    if (9 == sscanf(s.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d%c%2d:%2d", &t.tm_year,
                    &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec,
                    &sign, &tzHours, &tzMinutes))
    {
      t.tm_year -= 1900;
      t.tm_mon -= 1;
      time_t const tt = my_timegm(&t);
      // Fix timezone offset
      if (sign == '-')
        res = tt + tzHours * 3600 + tzMinutes * 60;
      else if (sign == '+')
        res = tt - tzHours * 3600 - tzMinutes * 60;
    }
  }

  return res;
}

}
