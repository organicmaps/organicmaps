#include "base/timer.hpp"
#include "base/assert.hpp"
#include "base/macros.hpp"
#include "base/timegm.hpp"

#include "std/target_os.hpp"
#include "std/systime.hpp"
#include "std/cstdio.hpp"
#include "std/sstream.hpp"
#include "std/iomanip.hpp"
#include "std/algorithm.hpp"

namespace my
{

Timer::Timer(bool start/* = true*/)
{
  if (start)
    Reset();
}

// static
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
  ::gettimeofday(&tv, 0);
  return tv.tv_sec + tv.tv_usec / 1000000.0;
#endif
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

uint32_t GenerateTimestamp(int year, int month, int day)
{
  return (year - 100) * 10000 + (month + 1) * 100 + day;
}

uint32_t TodayAsYYMMDD()
{
  time_t rawTime = time(NULL);
  tm const * const pTm = gmtime(&rawTime);
  CHECK(pTm, ("Can't get current date."));
  return GenerateTimestamp(pTm->tm_year, pTm->tm_mon, pTm->tm_mday);
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

namespace
{

bool IsValid(tm const & t)
{
  /// @todo Funny thing, but "00" month is accepted as valid in get_time function.
  /// Seems like a bug in the std library.
  return (t.tm_mday >= 1 && t.tm_mday <= 31 &&
          t.tm_mon >= 0 && t.tm_mon <= 11);
}

}

time_t StringToTimestamp(string const & s)
{
  // Return current time in the case of failure
  time_t res = INVALID_TIME_STAMP;

  if (s.size() == 20)
  {
    // Parse UTC format: 1970-01-01T00:00:00Z
    tm t{};
    istringstream ss(s);
    ss >> get_time(&t, "%Y-%m-%dT%H:%M:%SZ");

    if (!ss.fail() && IsValid(t))
      res = base::TimeGM(t);
  }
  else if (s.size() == 25)
  {
    // Parse custom time zone offset format: 2012-12-03T00:38:34+03:30
    tm t1{}, t2{};
    char sign;
    istringstream ss(s);
    ss >> get_time(&t1, "%Y-%m-%dT%H:%M:%S") >> sign >> get_time(&t2, "%H:%M");

    if (!ss.fail() && IsValid(t1))
    {
      time_t const tt = base::TimeGM(t1);

      // Fix timezone offset
      if (sign == '-')
        res = tt + t2.tm_hour * 3600 + t2.tm_min * 60;
      else if (sign == '+')
        res = tt - t2.tm_hour * 3600 - t2.tm_min * 60;
    }
  }

  return res;
}

HighResTimer::HighResTimer(bool start/* = true*/)
{
  if (start)
    Reset();
}

void HighResTimer::Reset()
{
  m_start = high_resolution_clock::now();
}

uint64_t HighResTimer::ElapsedNano() const
{
  return duration_cast<nanoseconds>(high_resolution_clock::now() - m_start).count();
}

uint64_t HighResTimer::ElapsedMillis() const
{
  return duration_cast<milliseconds>(high_resolution_clock::now() - m_start).count();
}

double HighResTimer::ElapsedSeconds() const
{
  return duration_cast<duration<double>>(high_resolution_clock::now() - m_start).count();
}

}
