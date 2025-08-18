#include "base/timer.hpp"

#include "base/assert.hpp"
#include "base/gmtime.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/timegm.hpp"

#include "std/target_os.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <iomanip>  // std::get_time
#include <sstream>

namespace base
{
// static
double Timer::LocalTime()
{
  auto const now = std::chrono::system_clock::now();
  return std::chrono::duration<double>(now.time_since_epoch()).count();
}

std::string FormatCurrentTime()
{
  time_t t = time(NULL);
  std::string s(ctime(&t));

  replace(s.begin(), s.end(), ' ', '_');

  ASSERT_GREATER(s.size(), 1, ());
  s.resize(s.size() - 1);
  return s;
}

uint32_t GenerateYYMMDD(int year, int month, int day)
{
  uint32_t result = (year - 100) * 100;
  result = (result + month + 1) * 100;
  result = result + day;
  return result;
}

uint32_t GenerateYYMMDD(uint64_t secondsSinceEpoch)
{
  auto const tm = GmTime(SecondsSinceEpochToTimeT(secondsSinceEpoch));
  return GenerateYYMMDD(tm.tm_year, tm.tm_mon, tm.tm_mday);
}

uint64_t YYMMDDToSecondsSinceEpoch(uint32_t yymmdd)
{
  auto constexpr partsCount = 3;
  // From left to right YY MM DD.
  std::array<int, partsCount> parts{};  // Initialize with zeros.
  for (auto i = partsCount - 1; i >= 0; --i)
  {
    parts[i] = yymmdd % 100;
    yymmdd /= 100;
  }
  ASSERT_EQUAL(yymmdd, 0, ("Version is too big."));

  ASSERT_GREATER_OR_EQUAL(parts[1], 1, ("Month should be in range [1, 12]"));
  ASSERT_LESS_OR_EQUAL(parts[1], 12, ("Month should be in range [1, 12]"));
  ASSERT_GREATER_OR_EQUAL(parts[2], 1, ("Day should be in range [1, 31]"));
  ASSERT_LESS_OR_EQUAL(parts[2], 31, ("Day should be in range [1, 31]"));

  std::tm tm{};
  tm.tm_year = parts[0] + 100;
  tm.tm_mon = parts[1] - 1;
  tm.tm_mday = parts[2];

  return TimeTToSecondsSinceEpoch(TimeGM(tm));
}

uint64_t SecondsSinceEpoch()
{
  return TimeTToSecondsSinceEpoch(::time(nullptr));
}

std::string TimestampToString(time_t time)
{
  if (time == INVALID_TIME_STAMP)
    return std::string("INVALID_TIME_STAMP");

  tm * t = gmtime(&time);
  char buf[100];
#ifdef OMIM_OS_WINDOWS
  sprintf_s(buf, ARRAY_SIZE(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);
#else
  ::snprintf(buf, ARRAY_SIZE(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);
#endif

  return buf;
}

std::string SecondsSinceEpochToString(uint64_t secondsSinceEpoch)
{
  return TimestampToString(SecondsSinceEpochToTimeT(secondsSinceEpoch));
}

time_t StringToTimestamp(std::string const & s)
{
  auto const size = s.size();
  if (size < 19)
    return INVALID_TIME_STAMP;

  // Parse UTC format prefix: 1970-01-01T00:00:00
  tm t{};
  std::istringstream ss(s);
  ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S");

  if (ss.fail())
    return INVALID_TIME_STAMP;

  time_t res = TimeGM(t);

  if (size == 19)
    return res;  // 1970-01-01T00:00:00

  char signOrDotOrZ;
  ss >> signOrDotOrZ;

  if (signOrDotOrZ == 'Z')
    return res;  // 1970-01-01T00:00:00Z

  // Fractional second values are dropped, as POSIX time_t doesn't hold them.
  if (signOrDotOrZ == '.')
  {
    while (ss >> signOrDotOrZ && std::isdigit(signOrDotOrZ))
    {}

    if (ss.fail())
      return INVALID_TIME_STAMP;

    if (signOrDotOrZ == 'Z')
      return res;  // 1970-01-01T00:00:00.123Z
  }

  if (signOrDotOrZ != '-' && signOrDotOrZ != '+')
    return INVALID_TIME_STAMP;

  // Parse custom time zone offset format: 2012-12-03T00:38:34+03:30
  ss >> std::get_time(&t, "%H:%M");

  if (ss.fail())
    return INVALID_TIME_STAMP;

  // Fix timezone offset
  if (signOrDotOrZ == '-')
    res = res + t.tm_hour * 3600 + t.tm_min * 60;
  else  // Positive offset should be subtracted.
    res = res - t.tm_hour * 3600 - t.tm_min * 60;

  return res;
}

time_t SecondsSinceEpochToTimeT(uint64_t secondsSinceEpoch)
{
  std::chrono::time_point<std::chrono::system_clock> const tpoint{std::chrono::seconds(secondsSinceEpoch)};
  return std::chrono::system_clock::to_time_t(tpoint);
}

uint64_t TimeTToSecondsSinceEpoch(time_t time)
{
  auto const tpoint = std::chrono::system_clock::from_time_t(time);
  return std::chrono::duration_cast<std::chrono::seconds>(tpoint.time_since_epoch()).count();
}

ScopedTimerWithLog::ScopedTimerWithLog(std::string const & timerName, Measure measure)
  : m_name(timerName)
  , m_measure(measure)
{}

ScopedTimerWithLog::~ScopedTimerWithLog()
{
  switch (m_measure)
  {
  case Measure::MilliSeconds:
  {
    LOG(LINFO, (m_name, "time:", m_timer.ElapsedMilliseconds(), "ms"));
    return;
  }
  case Measure::Seconds:
  {
    LOG(LINFO, (m_name, "time:", m_timer.ElapsedSeconds(), "s"));
    return;
  }
  }
  UNREACHABLE();
}
}  // namespace base
