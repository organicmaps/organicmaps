#pragma once

#include "3party/opening_hours/opening_hours.hpp"

#if defined(OMIM_OS_MAC) || defined(OMIM_OS_LINUX)
#include <sys/resource.h>
#endif

#include <cstdint>

namespace platform::tests_support
{
inline void ChangeMaxNumberOfOpenFiles(size_t n)
{
#if defined(OMIM_OS_MAC) || defined(OMIM_OS_LINUX)
  struct rlimit rlp;
  getrlimit(RLIMIT_NOFILE, &rlp);
  rlp.rlim_cur = n;
  setrlimit(RLIMIT_NOFILE, &rlp);
#endif
}

using Month = osmoh::MonthDay::Month;
using Weekday = osmoh::Weekday;

time_t GetUnixtimeByDate(uint16_t year, Month month, uint8_t monthDay, uint8_t hours, uint8_t minutes);
time_t GetUnixtimeByWeekday(uint16_t year, Month month, Weekday weekday, uint8_t hours, uint8_t minutes);
}  // namespace platform::tests_support
